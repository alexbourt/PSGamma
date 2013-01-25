#include "PIDefines.h"

#ifdef __PIMac__
	#include <Carbon.r>
	#include "PIGeneral.r"
	#include "PSGammaScripting.h"
	#include "PIUtilities.r"
#elif defined(__PIWin__)
	#define Rez
	#include "PSGammaScripting.h"
	#include "PIGeneral.h"
	#include "PIUtilities.r"
#endif

#include "PIActions.h"

resource 'PiPL' ( 16000, "PSGamma", purgeable )
{
	{
		Kind { Filter },
		Name { plugInName "..." },
		Category { vendorName },
		Version { (latestFilterVersion << 16 ) | latestFilterSubVersion },

		#ifdef __PIMac__
			
			#if (defined(__i386__))
				
				CodeMacIntel32 { "PluginMain" },
				
				/* If your plugin uses Objective-C, Cocoa, for UI it should not be
				   unloaded, this flag is valid for 32 bit plug-ins only and
			       should not be used in any windows section */
			       
				// off for now as this plug-in has no Objective-C Cocoa {}, 
			
			#endif
			#if (defined(__x86_64__))
				CodeMacIntel64 { "PluginMain" },
			#endif
		#else
			#if defined(_WIN64)
				CodeWin64X86 { "PluginMain" },
			#else
				CodeWin32X86 { "PluginMain" },
			#endif
		#endif

		SupportedModes
		{
			noBitmap, doesSupportGrayScale,
			noIndexedColor, doesSupportRGBColor,
			doesSupportCMYKColor, doesSupportHSLColor,
			doesSupportHSBColor, doesSupportMultichannel,
			doesSupportDuotone, doesSupportLABColor
		},

		HasTerminology
		{
			plugInClassID,
			plugInEventID,
			16000,
			plugInUniqueID
		},
		
		EnableInfo { "in (PSHOP_ImageMode, RGBMode, GrayScaleMode,"
		             "CMYKMode, HSLMode, HSBMode, MultichannelMode,"
					 "DuotoneMode, LabMode, RGB48Mode, Gray16Mode) ||"
					 "PSHOP_ImageDepth == 16 ||"
					 "PSHOP_ImageDepth == 32" },

		PlugInMaxSize { 2000000, 2000000 },
		
		FilterLayerSupport {doesSupportFilterLayers},
		
		FilterCaseInfo
		{
			{
				/* Flat data, no selection */
				inWhiteMat, outWhiteMat,
				doNotWriteOutsideSelection,
				filtersLayerMasks, worksWithBlankData,
				copySourceToDestination,
					
				/* Flat data with selection */
				inWhiteMat, outWhiteMat,
				writeOutsideSelection,
				filtersLayerMasks, worksWithBlankData,
				copySourceToDestination,
				
				/* Floating selection */
				inWhiteMat, outWhiteMat,
				writeOutsideSelection,
				filtersLayerMasks, worksWithBlankData,
				copySourceToDestination,
					
				/* Editable transparency, no selection */
				inWhiteMat, outWhiteMat,
				doNotWriteOutsideSelection,
				filtersLayerMasks, worksWithBlankData,
				copySourceToDestination,
					
				/* Editable transparency, with selection */
				inWhiteMat, outWhiteMat,
				writeOutsideSelection,
				filtersLayerMasks, worksWithBlankData,
				copySourceToDestination,
					
				/* Preserved transparency, no selection */
				inWhiteMat, outWhiteMat,
				doNotWriteOutsideSelection,
				filtersLayerMasks, worksWithBlankData,
				copySourceToDestination,
					
				/* Preserved transparency, with selection */
				inWhiteMat, outWhiteMat,
				writeOutsideSelection,
				filtersLayerMasks, worksWithBlankData,
				copySourceToDestination
			}
		}	
	}
};

resource 'aete' (16000, "Gamma dictionary", purgeable)
{
	1, 0, english, roman,									/* aete version and language specifiers */
	{
		vendorName,											/* vendor suite name */
		"Bourt plug-ins",							/* optional description */
		plugInSuiteID,										/* suite ID */
		1,													/* suite code, must be 1 */
		1,													/* suite level, must be 1 */
		{													/* structure for filters */
			plugInName,										/* unique filter name */
			plugInAETEComment,								/* optional description */
			plugInClassID,									/* class ID, must be unique or Suite ID */
			plugInEventID,									/* event ID, must be unique to class ID */
			
			NO_REPLY,										/* never a reply */
			IMAGE_DIRECT_PARAMETER,							/* direct parameter, used by Photoshop */
			{												/* parameters here, if any */
				"gamma",									/* parameter name */
				keyPSGamma,									/* parameter key ID */
				typeFloat,									/* parameter type ID */
				"gamma value",								/* optional description */
				flagsSingleParameter,						/* parameter flags */
				
				"invert",									/* second parameter */
				keyPSInvert,								/* key ID */
				typeBoolean,								/* type */
				"invert gamma value",						/* optional desc */
				flagsSingleParameter						/* parameter flags */
			}
		},
		{													/* non-filter plug-in class here */
		},
		{													/* comparison ops (not supported) */
		},
		{													/* any enumerations */
		}
	}
};
