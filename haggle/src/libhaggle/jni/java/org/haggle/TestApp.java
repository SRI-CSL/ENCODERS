package org.haggle;

import java.lang.System;
import java.lang.Thread;
import org.haggle.Handle;
import org.haggle.EventHandler;
import org.haggle.DataObject;
import org.haggle.LaunchCallback;

public class TestApp implements EventHandler {	
        private Handle h = null;
        private String name;
	private long num_dataobjects_received = 0;
	private boolean should_quit = false;
	private int num_dataobjects = 10;

	public synchronized void onNewDataObject(DataObject dObj) {
		num_dataobjects_received++;
                System.out.println("Got data object " + num_dataobjects_received + " filepath=" + dObj.getFilePath());
		dObj.dispose();
        }
	public synchronized void onNeighborUpdate(Node[] neighbors) {
                System.out.println("Got neighbor update event");

                for (int i = 0; i < neighbors.length; i++) {
                        System.out.println("Neighbor: " + neighbors[i].getName());
                }
        }
        public synchronized void onInterestListUpdate(Attribute[] interests) {
                System.out.println("Got interest list update event");

                for (int i = 0; i < interests.length; i++) {
                        System.out.println("Interest: " + interests[i].toString());
                }
        }
	public synchronized void onShutdown(int reason) {
                System.out.println("Got shutdown event, reason=" + reason);
		should_quit = true;
        }
	public void onEventLoopStart() {
		System.out.println("Event loop started");
	}
	public void onEventLoopStop() {
		System.out.println("Event loop stopped");
	}
        public TestApp(String name, int num_dataobjects)
        {
                super();
                this.name = name;
		this.num_dataobjects = num_dataobjects;
        }
        public void start()
        {
                Attribute attr = new Attribute("foo", "bar", 3);
                
                System.out.println("Attribute " + attr.toString());
                
                long pid = Handle.getDaemonPid();

                System.out.println("Pid is " + pid);

                if (pid == 0) {
                        // Haggle is not running
                        if (Handle.spawnDaemon(new LaunchCallback() { 
                                        public int callback(long milliseconds) {
                                                System.out.println("Launching haggle, milliseconds=" + milliseconds); 
                                                return 0;
                                        }
                                }) == false) {
                                // Could not spawn daemon
                                System.out.println("Could not spawn new Haggle daemon");
                                return;
                        }
                }
                try {
			DataObject[] dobjs = new DataObject[num_dataobjects];
                        h = new Handle(name);

                        h.registerEventInterest(EVENT_NEW_DATAOBJECT, this);
                        h.registerEventInterest(EVENT_NEIGHBOR_UPDATE, this);
                        h.registerEventInterest(EVENT_INTEREST_LIST_UPDATE, this);
                        h.registerEventInterest(EVENT_HAGGLE_SHUTDOWN, this);
                        
                        h.registerInterest(attr);
                
                        h.eventLoopRunAsync(this);
                        
			System.out.println("Getting Application interests");

                        h.getApplicationInterestsAsync();

                        Thread.sleep(2000);
			
			System.out.println("Forcing Haggle to send node description");

			h.sendNodeDescription();
			
			Thread.sleep(3000);

			for (int i = 0; i < num_dataobjects; i++) {
				dobjs[i] = new DataObject();
				dobjs[i].addAttribute("num", "" + i);
				dobjs[i].addAttribute(attr);
				h.publishDataObject(dobjs[i]);
				Thread.sleep(40);
			}
			
			Thread.sleep(2000);
			
			for (int i = 0; i < num_dataobjects; i++) {
				h.deleteDataObject(dobjs[i], false);
				dobjs[i].dispose();
				Thread.sleep(40);
			}

			while (num_dataobjects != num_dataobjects_received && !should_quit)
				Thread.sleep(1000);

                        h.eventLoopStop();
                        
                        Thread.sleep(2000);
			h.shutdown();
			Thread.sleep(2000);

                } catch (Handle.RegistrationFailedException e) {
                        System.out.println("Could not get handle: " + e.getMessage());
                        return;
                } catch (InterruptedException e) {
			h.unregister();
			h.eventLoopStop();
		} catch (Exception e) {
                        System.out.println("Got run loop exception\n");
                        return;
                }

                h.dispose();
        }
        public static void main(String args[])
        {
		int num_dataobjects = 10;

		for (int i = 0; i < args.length; i++) {
			if (args[i].compareTo("-h") == 0) {
				System.out.println("usage:");
				System.out.println("\t -h : print this help");
				System.out.println("\t -n NUM_DATAOBJECTS : generate specified number of data objects");
				System.out.println("");
				return;
			} else if (args[i].compareTo("-n") == 0 && (i + 1) != args.length) {
				try {
					num_dataobjects = Integer.parseInt(args[++i]);
					System.out.println("Going to generate " + num_dataobjects + " data objects");
				} catch (NumberFormatException e) {
					System.err.println("Bad number format in argument");
				}
			}
		}
                TestApp app = new TestApp("JavaTestApp", num_dataobjects);

                app.start();

                System.out.println("Done...\n");
        }
}
