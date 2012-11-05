//
//  Controller.h
//  SenderDemo
//
//  Created by Bernd Löhr on 15.11.11.
//  Copyright 2011 NewTec GmbH. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface Controller : NSResponder {

    // Sender:
	IBOutlet	id	textField;
	IBOutlet	id	sliderField1;
	IBOutlet	id	sliderField2;
	IBOutlet	id	ipAddress;

    // Receiver
    IBOutlet    id  rec1IP;
    IBOutlet    id  rec1ComID;
    IBOutlet    id  rec1Color;
    IBOutlet    id  rec1Count;
    IBOutlet    id  rec1Message;
    IBOutlet    id  rec2IP;
    IBOutlet    id  rec2ComID;
    IBOutlet    id  rec2Color;
    IBOutlet    id  rec2Count;
    IBOutlet    id  rec2Message;
    IBOutlet    id  rec3IP;
    IBOutlet    id  rec3ComID;
    IBOutlet    id  rec3Color;
    IBOutlet    id  rec3Count;
    IBOutlet    id  rec3Message;
    IBOutlet    id  rec4IP;
    IBOutlet    id  rec4ComID;
    IBOutlet    id  rec4Color;
    IBOutlet    id  rec4Count;
    IBOutlet    id  rec4Message;
    IBOutlet    id  rec5IP;
    IBOutlet    id  rec5ComID;
    IBOutlet    id  rec5Color;
    IBOutlet    id  rec5Bar;
    
    
	Boolean	isActive;
	
	NSTimer *timer;
	
	uint32_t	dataArray[5];
	Boolean		dataChanged;
}

- (IBAction) button1:(id)sender;
- (IBAction) button2:(id)sender;
- (IBAction) button3:(id)sender;
- (IBAction) slider1:(id)sender;
- (IBAction) slider2:(id)sender;
- (IBAction) enable1:(id)sender;
- (IBAction) ipChanged:(id)sender;
- (IBAction) comIDChanged:(id)sender;
- (IBAction) intervalChanged:(id)sender;

- (IBAction) ipChangedRec1:(id)sender;
- (IBAction) comIDChangedRec1:(id)sender;
- (IBAction) ipChangedRec2:(id)sender;
- (IBAction) comIDChangedRec2:(id)sender;
- (IBAction) ipChangedRec3:(id)sender;
- (IBAction) comIDChangedRec3:(id)sender;
- (IBAction) ipChangedRec4:(id)sender;
- (IBAction) comIDChangedRec4:(id)sender;
- (IBAction) ipChangedRec5:(id)sender;
- (IBAction) comIDChangedRec5:(id)sender;

@end
