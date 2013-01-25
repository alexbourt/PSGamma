#include "PSGammaScripting.h"

//-------------------------------------------------------------------------------
//
// ReadScriptParameters
//
// See if we were called by the Photoshop scripting system and return the value
// in displayDialog if the user wants to see our dialog.
// 
//-------------------------------------------------------------------------------
OSErr ReadScriptParameters(Boolean* displayDialog)
{
	OSErr err = noErr;
	PIReadDescriptor token = NULL;
	DescriptorKeyID key = 0;
	DescriptorTypeID type = 0;
	int32 flags = 0;
	DescriptorKeyIDArray array = { keyPSGamma, keyPSInvert, 0 };
	double gamma;
	Boolean invert;

	if (displayDialog != NULL)
		*displayDialog = gData->queryForParameters;
	else
		return errMissingParameter;

	if (!*displayDialog)
	{
		PIDescriptorParameters* descParams = gFilterRecord->descriptorParameters;
		if (descParams == NULL) return err;
	
		ReadDescriptorProcs* readProcs = gFilterRecord->descriptorParameters->readDescriptorProcs;
		if (readProcs == NULL) return err;
	
		if (descParams->descriptor != NULL)
		{
			token = readProcs->openReadDescriptorProc(descParams->descriptor, array);
			if (token != NULL)
			{
				while(readProcs->getKeyProc(token, &key, &type, &flags) && !err)
				{
					switch (key)
					{
						case keyPSGamma:
							err = readProcs->getFloatProc(token, &gamma);
							if (!err)
								gParams->gamma = gamma;
							break;

						case keyPSInvert:
							err = readProcs->getBooleanProc(token, &invert);
							if (!err)
								gParams->invert = invert != FALSE;
							break;

						default:
							err = readErr;
							break;
					}
				}
				err = readProcs->closeReadDescriptorProc(token);
				gFilterRecord->handleProcs->disposeProc(descParams->descriptor);
				descParams->descriptor = NULL;
			}
			*displayDialog = descParams->playInfo == plugInDialogDisplay;
		}
	}

	return err;
}



//-------------------------------------------------------------------------------
//
// WriteScriptParameters
//
// Write our parameters to the Photoshop scripting system in case we are being
// recorded in the actions pallete.
// 
//-------------------------------------------------------------------------------
OSErr WriteScriptParameters(void)
{
	OSErr err = noErr;
	PIWriteDescriptor token = NULL;
	PIDescriptorHandle h;

	const double gamma = gParams->gamma;

	PIDescriptorParameters*	descParams = gFilterRecord->descriptorParameters;
	if (descParams == NULL) return err;
	
	WriteDescriptorProcs* writeProcs = gFilterRecord->descriptorParameters->writeDescriptorProcs;
	if (writeProcs == NULL) return err;

	token = writeProcs->openWriteDescriptorProc();
	if (token != NULL)
	{
		writeProcs->putFloatProc(token, 
			                     keyPSGamma, 
								 &gamma);

		if (gParams->invert)
			writeProcs->putBooleanProc(token, 
			                           keyPSInvert, 
									   gParams->invert);

		gFilterRecord->handleProcs->disposeProc(descParams->descriptor);
		writeProcs->closeWriteDescriptorProc(token, &h);
		descParams->descriptor = h;
		descParams->recordInfo = plugInDialogOptional;
	}
	else
	{
		return errMissingParameter;
	}
	return err;
}