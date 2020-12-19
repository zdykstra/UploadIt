/*
 * Copyright 2018-2020. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *	Humdinger, humdingerb@gmail.com
 *
 * Modified code from CopyNameToClipboard
 * _CheckNetworkConnection() borrowed from SoftwareUpdater
 */

#include <stdio.h>
#include <stdlib.h>

#include <Alert.h>
#include <Catalog.h>
#include <Clipboard.h>
#include <Entry.h>
#include <NetworkInterface.h>
#include <NetworkRoster.h>
#include <Path.h>
#include <String.h>
#include <Notification.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Add-On"


bool
CheckNetworkConnection()
{
	BNetworkRoster& roster = BNetworkRoster::Default();
	BNetworkInterface interface;
	uint32 cookie = 0;
	while (roster.GetNextInterface(&cookie, interface) == B_OK) {
		uint32 flags = interface.Flags();
		if ((flags & IFF_LOOPBACK) == 0
			&& (flags & (IFF_UP | IFF_LINK)) == (IFF_UP | IFF_LINK)) {
			return true;
		}
	}
	// No network connection detected, cannot continue
	return false;
}


void
CopyToClipboard(BString text)
{
	ssize_t textLen = text.Length();
	BMessage* message = (BMessage *)NULL;

	if (be_clipboard->Lock()) {
		be_clipboard->Clear();
		if ((message = be_clipboard->Data())) {
			message->AddData("text/plain", B_MIME_TYPE, text.String(),
				textLen);
			be_clipboard->Commit();
		}
		be_clipboard->Unlock();
	}
}

BString 
GetStdoutFromCommand(BString cmd) {

  BString data;
  FILE * stream;
  const int max_buffer = 256;
  char buffer[max_buffer];

  stream = popen(cmd.String(), "r");

  if (stream) {
    while (!feof(stream))
      if (fgets(buffer, max_buffer, stream) != NULL) data.Append(buffer);
    pclose(stream);
  }
  return data;
}
extern "C" void
process_refs(entry_ref directoryRef, BMessage* msg, void*)
{
	BPath path;
	entry_ref file_ref;
	BNotification notification(B_INFORMATION_NOTIFICATION);
	notification.SetTitle("UploadIt");
	notification.SetMessageID("UploadIt");

	if (msg->FindRef("refs", &file_ref) == B_NO_ERROR) {
		BEntry entry(&file_ref);
		if (CheckNetworkConnection() == false) {
			BString text(B_TRANSLATE(
				"Online upload service not available"));
			CopyToClipboard(text);
		} else {
			entry.GetPath(&path);
			BString starting(B_TRANSLATE("Uploading '%FILE%'" B_UTF8_ELLIPSIS));
			starting.ReplaceAll("%FILE%", path.Leaf());
			CopyToClipboard(starting);

			notification.SetContent(starting);
			notification.Send();

			BString command("webify -p %FILEPATH%");
			command.ReplaceFirst("%FILEPATH%", path.Path());
			BString output = GetStdoutFromCommand(command.String());
			output.ReplaceLast("\n","");

			CopyToClipboard(output);

			BString finished(B_TRANSLATE("Finished uploading '%FILE%'" B_UTF8_ELLIPSIS));
			finished.ReplaceAll("%FILE%", path.Leaf());
			notification.SetOnClickApp("application/x-vnd.Haiku-WebPositive");
			notification.AddOnClickArg(output.String());
			notification.SetContent(finished);
			notification.Send();
		}
	}
}


int
main(int, char**)
{
	fprintf(stderr, B_TRANSLATE("UploadIt can only be used as a Tracker add-on.\n"));
	return -1;
}
