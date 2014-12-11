package org.haggle.PhotoShare;

import org.haggle.Interface;
import org.haggle.Node;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

class NodeAdapter extends BaseAdapter {
	private Context mContext;
	private Node[] neighbors = null;

	public NodeAdapter(Context mContext) {
		this.mContext = mContext;
	}
	public int getCount() {
		if (neighbors == null || neighbors.length == 0)
			return 1;
		else
			return neighbors.length;
	}

	public Object getItem(int position) {
		return position;
	}

	public long getItemId(int position) {
		return position;
	}
	public synchronized void updateNeighbors(Node[] neighs) {
		neighbors = neighs;

		notifyDataSetChanged();
	}

    public void refresh() {
    	notifyDataSetChanged();
    }
    
	public View getView(int position, View convertView, ViewGroup parent) {    		
		TextView tv;
		
		if (convertView == null) {
            tv = (TextView) LayoutInflater.from(mContext).inflate(R.layout.neighbor_list_item, parent, false);
        } else {
            tv = (TextView) convertView;
        }

		if (neighbors == null || neighbors.length == 0) {
			tv.setText("No active neighbors");

		} else {
			Node node = neighbors[position];
			
			String ifaceInfo = new String(" [");
			for (int i=0; i<node.getNumInterfaces(); i++) {
				Interface iface = node.getInterfaceN(i);
				if (iface != null) {
    				if (iface.getType() == Interface.IFTYPE_BLUETOOTH && iface.getStatus() == Interface.IFSTATUS_UP) {
    					ifaceInfo += " BT";
    				}
    				if (iface.getType() == Interface.IFTYPE_ETHERNET && iface.getStatus() == Interface.IFSTATUS_UP) {
    					ifaceInfo += " WiFi";
    				}
				}
			}
			ifaceInfo += " ]";
			tv.setText(node.getName() + ifaceInfo);
		}
		return tv;
	}
	public Node getNode(int pos) {
		if (pos < 0 || pos > neighbors.length - 1) 
			return null;
		
		return neighbors[pos];
	}
	public synchronized Node[] getNodes() {
		return neighbors.clone();
	}
	public String getInformation(int position) {
		if ((neighbors.length > 0) && (position >= 0)) {
			Node n = neighbors[position];
			return n.toString();
		} else {
			return "Node Information";
		}
	}
}
