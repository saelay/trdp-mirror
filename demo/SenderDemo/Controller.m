//
//  Controller.m
//  SenderDemo
//
//  Created by Bernd Löhr on 15.11.11.
//  Copyright 2011 NewTec GmbH. All rights reserved.
//

#import "Controller.h"
#include "pdsend.h"

extern int gIsActive;

@implementation Controller

- (IBAction)button1:(id)sender 
{
	[textField setStringValue:@"Toggle Door1 command"];
	dataArray[0] = htonl(!ntohl(dataArray[0]));
	dataChanged = true;
	pd_updateData((uint8_t*)dataArray, sizeof(dataArray));
}
- (IBAction)button2:(id)sender 
{
	[textField setStringValue:@"Toggle Door2 command"];
	dataArray[1] = htonl(!ntohl(dataArray[1]));
	dataChanged = true;
	pd_updateData((uint8_t*)dataArray, sizeof(dataArray));
}
- (IBAction)button3:(id)sender 
{
	[textField setStringValue:@"Toggle Light command"];
	dataArray[2] = htonl(!ntohl(dataArray[2]));
	dataChanged = true;
	pd_updateData((uint8_t*)dataArray, sizeof(dataArray));
}
- (IBAction)slider1:(id)sender 
{
	uint32_t	val = (uint32_t)[sender intValue];
	[sliderField1 setIntValue:val];
	dataArray[3] = htonl(val);
	dataChanged = true;
	pd_updateData((uint8_t*)dataArray, sizeof(dataArray));
}
- (IBAction)slider2:(id)sender 
{
	uint32_t	val = (uint32_t)[sender intValue];
	[sliderField2 setIntValue:val];
	dataArray[4] = htonl(val);
	dataChanged = true;
	pd_updateData((uint8_t*)dataArray, sizeof(dataArray));
}

- (IBAction) ipChanged:(id)sender
{
	setIP((char*)[[ipAddress stringValue] UTF8String]);
	dataChanged = true;
	pd_updateData((uint8_t*)dataArray, sizeof(dataArray));
}

- (IBAction) enable1:(id)sender
{
	if ([sender isSelectedForSegment:0])
	{
		printf("enable\n");
		isActive = true;
		gIsActive = 1;
	}
	else
	{
		printf("disable\n");
		isActive = false;
		gIsActive = 0;
	}
	dataChanged = true;
    pd_stop(!gIsActive);
}

- (IBAction) comIDChanged:(id)sender
{
	setComID((uint32_t)[sender intValue]);
}
- (IBAction) intervalChanged:(id)sender
{
	setInterval((uint32_t)[sender intValue]);
}

- (void)doIt
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
    printf("doIt\n");
	
	
	pd_init((char*)[[ipAddress stringValue] UTF8String], PD_COMID0, PD_COMID0_CYCLE);
	
	pd_loop2(); 

	pd_deinit();

    [pool release];
}

//  Event driven update function for the views

- (void)timerFired:(NSTimer *)timer
{
    pd_receive_packet_t*    current = NULL;
    
    // Animate view for now
	if (dataChanged)
	{
		pd_updateData((uint8_t*)dataArray, sizeof(dataArray));
		dataChanged = false;
	}

    // Update our receive views
    current = pd_get(0);
    if (current != NULL && current->changed)
    {
        if (current->invalid)
        {
            [rec1Color setColor:[NSColor redColor]];
        }
        else
        {
            [rec1Color setColor:[NSColor greenColor]];
        }
        [rec1Count setIntValue:current->counter];
        [rec1Message setStringValue:[NSString stringWithUTF8String:(const char*)current->message]];
        current->changed = 0;
    }

    current = pd_get(1);
    if (current != NULL && current->changed)
    {
        if (current->invalid)
        {
            [rec2Color setColor:[NSColor redColor]];
        }
        else
        {
            [rec2Color setColor:[NSColor greenColor]];
        }
        [rec2Count setIntValue:current->counter];
        [rec2Message setStringValue:[NSString stringWithUTF8String:(const char*)current->message]];
        current->changed = 0;
    }

    current = pd_get(2);
    if (current != NULL && current->changed)
    {
        if (current->invalid)
        {
            [rec3Color setColor:[NSColor redColor]];
        }
        else
        {
            [rec3Color setColor:[NSColor greenColor]];
        }
        [rec3Count setIntValue:current->counter];
        [rec3Message setStringValue:[NSString stringWithUTF8String:(const char*)current->message]];
        current->changed = 0;
    }

    current = pd_get(3);
    if (current != NULL && current->changed)
    {
        if (current->invalid)
        {
            [rec4Color setColor:[NSColor redColor]];
        }
        else
        {
            [rec4Color setColor:[NSColor greenColor]];
        }
        [rec4Count setIntValue:current->counter];
        [rec4Message setStringValue:[NSString stringWithUTF8String:(const char*)current->message]];
        current->changed = 0;
    }

    current = pd_get(4);
    if (current != NULL && current->changed)
    {
        if (current->invalid)
        {
            [rec5Color setColor:[NSColor redColor]];
            //[rec5Bar setColor:[NSColor grayColor]];
        }
        else
        {
            [rec5Color setColor:[NSColor greenColor]];
        }
        [rec5Bar setIntValue:current->counter];
        current->changed = 0;
    }
}

#if 1
- (void)doViewUpdates
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
    printf("doViewUpdates\n");
	
	while (1)
    {
        [NSThread sleepForTimeInterval:0.05];
        [self timerFired:nil];
    }
    
    [pool release];
}
#endif

- (void)applicationWillFinishLaunching:(NSNotification *)note
{
	dataChanged = false;
	[NSThread detachNewThreadSelector:@selector(doIt) toTarget:self withObject:self];

#if 1
    // Create timer for updating the receive views with NSThread
	[NSThread detachNewThreadSelector:@selector(doViewUpdates) toTarget:self withObject:self];
#else
    // Create timer event for updating the receive views
    timer = [[NSTimer scheduledTimerWithTimeInterval:0.05f target:self 
                                            selector:@selector(timerFired:) userInfo:nil repeats:YES] retain];
#endif
    printf("applicationWillFinishLaunching\n");

}

- (void)applicationWillTerminate:(NSNotification *)note
{
}

@end
