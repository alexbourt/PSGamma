#include "PSGammaRegistry.h"

//-------------------------------------------------------------------------------
//
// ReadRegistryParameters
//
// Read our parameters out of the Photoshop Registry. This saves us from writing
// to a preferences file and the pains of managing that on each OS. You should 
// probably ignore errors from this routine as other hosts have not 
// implemented a registry.
//
//-------------------------------------------------------------------------------
SPErr ReadRegistryParameters(void)
{
	SPErr err = noErr;
	SPBasicSuite* basicSuite = gFilterRecord->sSPBasic;
	PSDescriptorRegistryProcs* registryProcs = NULL;
	PSActionDescriptorProcs* descriptorProcs = NULL;
	PIActionDescriptor descriptor = NULL;
	//double gamma;
	//Boolean invert;

	if (basicSuite == NULL)
		return errPlugInHostInsufficient;

	err = basicSuite->AcquireSuite(kPSDescriptorRegistrySuite, 
		                           kPSDescriptorRegistrySuiteVersion, 
								   (const void **)&registryProcs);
	if (err) goto returnError;

	err = basicSuite->AcquireSuite(kPSActionDescriptorSuite, 
		                           kPSActionDescriptorSuiteVersion, 
								   (const void **)&descriptorProcs);
	if (err) goto returnError;

	err = registryProcs->Get(plugInUniqueID, &descriptor);
	if (err) goto returnError;
	if (descriptor == NULL) goto returnError;

	//err = descriptorProcs->GetFloat(descriptor, 
	//	                            keyPSGamma, 
	//								&gamma);
	//if (err) goto returnError;
	//gParams->gamma = gamma;

	//err = descriptorProcs->GetBoolean(descriptor, 
	//	                              keyPSInvert, 
	//								  &invert);
	//if (err) goto returnError;

	//gParams->invert = invert != FALSE;

returnError:
	if (descriptor != NULL)
		descriptorProcs->Free(descriptor);
	if (registryProcs != NULL)
		basicSuite->ReleaseSuite(kPSDescriptorRegistrySuite, 
		                         kPSDescriptorRegistrySuiteVersion);
	if (descriptorProcs != NULL)
		basicSuite->ReleaseSuite(kPSActionDescriptorSuite, 
		                         kPSActionDescriptorSuiteVersion);
	return err;
}



//-------------------------------------------------------------------------------
//
// WriteRegistryParameters
//
// Write our parameters out to the Photoshop Registry. This saves us from writing
// to a preferences file and the pains of managing that on each OS. You should 
// probably ignore errors from this routine as other hosts have not 
// implemented a registry.
// 
//-------------------------------------------------------------------------------
SPErr WriteRegistryParameters(void)
{
	SPErr err = noErr;
	SPBasicSuite* basicSuite = gFilterRecord->sSPBasic;
	PSDescriptorRegistryProcs* registryProcs = NULL;
	PSActionDescriptorProcs* descriptorProcs = NULL;
	PIActionDescriptor descriptor = NULL;


	if (basicSuite == NULL)
		return errPlugInHostInsufficient;

	err = basicSuite->AcquireSuite(kPSDescriptorRegistrySuite, 
		                           kPSDescriptorRegistrySuiteVersion, 
								   (const void **)&registryProcs);
	if (err) goto returnError;

	err = basicSuite->AcquireSuite(kPSActionDescriptorSuite, 
		                           kPSActionDescriptorSuiteVersion, 
								   (const void **)&descriptorProcs);
	if (err) goto returnError;

	err = descriptorProcs->Make(&descriptor);
	if (err) goto returnError;

	err = descriptorProcs->PutFloat(descriptor, 
		                            keyPSGamma, 
									gParams->gamma);
	if (err) goto returnError;

	err = descriptorProcs->PutBoolean(descriptor, 
		                              keyPSInvert, 
									  (Boolean)gParams->invert);
	if (err) goto returnError;

	err = registryProcs->Register(plugInUniqueID, descriptor, true);
	if (err) goto returnError;

returnError:
	if (descriptor != NULL)
		descriptorProcs->Free(descriptor);
	if (registryProcs != NULL)
		basicSuite->ReleaseSuite(kPSDescriptorRegistrySuite, 
		                         kPSDescriptorRegistrySuiteVersion);
	if (descriptorProcs != NULL)
		basicSuite->ReleaseSuite(kPSActionDescriptorSuite, 
		                         kPSActionDescriptorSuiteVersion);
	return err;
}