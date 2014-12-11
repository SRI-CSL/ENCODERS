//
//  HagglePrint.m
//  HagglePrint
//
//  Created by Daniel Aldman on 2008-12-17.
//  Copyright 2008 Uppsala Universitet. All rights reserved.
//

#import "HagglePrint.h"
#include "haggle.h"

HagglePrint *thePrinter;
haggle_handle_t	_haggle;
BOOL haggle_initialized;

typedef struct do_table_s {
	NSString *file_path;
	NSString *file_name;
	NSString *action;
} *do_table;

long number_of_data_objects;
do_table data_object;

static void printDataObject(long which)
{
	char str[1024];
	
	sprintf(
		str, 
		"(echo \"tell application\\\"Preview\\\"\" ;"
		// FIXME: change "open" to "print" when ready.
		" echo \"    open \\\"%s\\\"\" ;"
		" echo \"    quit\" ;"
		" echo \"end tell\") | osascript -",
		[data_object[which].file_path 
			cStringUsingEncoding:NSMacOSRomanStringEncoding]);
	system(str);
}

static void openDataObject(long which)
{
	char str[1024];
	
	sprintf(
		str, 
		"open '%s'",
		[data_object[which].file_path 
			cStringUsingEncoding:NSMacOSRomanStringEncoding]);
	system(str);
}

static void onDataObject(struct dataobject *dObj, void *arg)
{
	printf("DING!\n");
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	struct attribute *attr = 
		haggle_dataobject_get_attribute_by_name(dObj, "Please");
	// Check to make sure there is a file and that it should be 
	// printed/projected
	if(	haggle_dataobject_get_filepath(dObj) != NULL &&
		attr != NULL)
	{
		do_table tmp;
		
		tmp = 
			realloc(
				data_object, 
				sizeof(struct do_table_s) * (number_of_data_objects + 1));
		if(tmp != NULL)
		{
			data_object = tmp;
			
			data_object[number_of_data_objects].file_path =
				[NSString 
					stringWithCString:
						haggle_dataobject_get_filepath(dObj)
					encoding:NSMacOSRomanStringEncoding];
			data_object[number_of_data_objects].file_name =
				[NSString 
					stringWithCString:
						haggle_dataobject_get_filename(dObj)
					encoding:NSMacOSRomanStringEncoding];
			if(strcmp(
				haggle_attribute_get_value(attr), 
				"PrintThis") == 0)
			{
				data_object[number_of_data_objects].action =
					[NSString 
						stringWithCString:"Print"
						encoding:NSMacOSRomanStringEncoding];
				[data_object[number_of_data_objects].file_path retain];
				[data_object[number_of_data_objects].file_name retain];
				[data_object[number_of_data_objects].action retain];
				number_of_data_objects++;
				if([thePrinter->printFiles state] == 1)
					printDataObject(number_of_data_objects-1);
			}else if(
				strcmp(
					haggle_attribute_get_value(attr), 
					"ProjectThis") == 0)
			{
				data_object[number_of_data_objects].action =
					[NSString 
						stringWithCString:"Project"
						encoding:NSMacOSRomanStringEncoding];
				[data_object[number_of_data_objects].file_path retain];
				[data_object[number_of_data_objects].file_name retain];
				[data_object[number_of_data_objects].action retain];
				number_of_data_objects++;
				if([thePrinter->projectFiles state] == 1)
					openDataObject(number_of_data_objects-1);
			}
		}
	}
	[thePrinter reloadData];
	[pool drain];
}

@implementation HagglePrint

- (void)reloadData
{
	[theTable reloadData];
}

- (void)awakeFromNib
{
	bool has_retried = false;
	int err;
	
	[theTable setDataSource:self];
	thePrinter = self;
	[theTable setTarget:self];
	[theTable setDoubleAction:@selector(myDoubleClickAction:)];
	haggle_initialized = NO;
	data_object = NULL;
	number_of_data_objects = 0;
retry:
	switch(err = haggle_handle_get("HagglePrint", &_haggle))
	{
		case HAGGLE_NO_ERROR:
			haggle_initialized = YES;
			
			if(haggle_ipc_register_event_interest(
					_haggle, 
					LIBHAGGLE_EVENT_NEW_DATAOBJECT, 
					onDataObject) <= 0)
				goto failed_to_init_haggle;
			if(haggle_event_loop_run_async(_haggle) != HAGGLE_NO_ERROR)
				goto failed_to_init_haggle;
		break;
		
		case HAGGLE_BUSY_ERROR:
			haggle_unregister("HagglePrint");
			if(!has_retried)
			{
				has_retried = true;
				goto retry;
			}
			goto failed_to_init_haggle;
		break;
		
		default:
			printf("err = %d\n", err);
failed_to_init_haggle:
			// FIXME: show some kind of alert first, telling the user haggle 
			// isn't running
			[NSApp terminate:self];
		break;
	}
}

- (void)applicationWillTerminate:(NSNotification *)notification
{
	long i;
	
	if(haggle_initialized)
	{
		if([getPrintFiles state] == 1)
			haggle_ipc_remove_application_interest(
				_haggle, 
				"Please", 
				"PrintThis");
		if([getProjectFiles state] == 1)
			haggle_ipc_remove_application_interest(
				_haggle, 
				"Please", 
				"ProjectThis");
		haggle_handle_free(_haggle);
	}
	if(data_object != NULL)
	{
		for(i = 0; i < number_of_data_objects; i++)
		{
			[data_object[i].file_name release];
			[data_object[i].file_path release];
			[data_object[i].action release];
		}
		free(data_object);
		number_of_data_objects = 0;
		data_object = NULL;
	}
}

- (IBAction) myDoubleClickAction:(id)sender
{
	NSIndexSet *currentSet = [theTable selectedRowIndexes];
	
	NSUInteger i;
	
	// There shoulnd't really be more than one index, but still...
	for(i = [currentSet firstIndex]; 
		i != NSNotFound; 
		i = [currentSet indexGreaterThanIndex:i])
	{
		if([data_object[i].action isEqualToString:@"Print"])
			printDataObject(i);
		if([data_object[i].action isEqualToString:@"Project"])
			openDataObject(i);
	}
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView
{
	return number_of_data_objects;
}

- (id)tableView:(NSTableView *)tableView 
		objectValueForTableColumn:(NSTableColumn *)tableColumn 
		row:(NSInteger)row
{
	if([[[tableColumn headerCell] title] isEqualToString:@"Filename"])
	{
		if(row < number_of_data_objects)
			return data_object[row].file_name;
		else
			return @"THIS IS A BUG!";
	}else if([[[tableColumn headerCell] title] isEqualToString:@"Action"])
	{
		if(row < number_of_data_objects)
			return data_object[row].action;
		else
			return @"THIS IS A BUG!";
	}else{
		printf("Unknown column!\n");
		return nil;
	}
}

- (IBAction) myClickCheckboxAction:(id)sender
{
	if(sender == getPrintFiles)
	{
		if([sender state] == 1)
		{
			if(haggle_ipc_add_application_interest(
				_haggle, 
				"Please", 
				"PrintThis") <= 0)
				[sender setState:0];
		}else{
			if(haggle_ipc_remove_application_interest(
				_haggle, 
				"Please", 
				"PrintThis") <= 0)
				[sender setState:1];
		}
	}else if(sender == getProjectFiles)
	{
		if([sender state] == 1)
		{
			if(haggle_ipc_add_application_interest(
				_haggle, 
				"Please", 
				"ProjectThis") <= 0)
				[sender setState:0];
		}else{
			if(haggle_ipc_remove_application_interest(
				_haggle, 
				"Please", 
				"ProjectThis") <= 0)
				[sender setState:1];
		}
	}
}
@end
