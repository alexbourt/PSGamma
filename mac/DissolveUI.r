// ADOBE SYSTEMS INCORPORATED
// Copyright  1993 - 2002 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE:  Adobe permits you to use, modify, and distribute this 
// file in accordance with the terms of the Adobe license agreement
// accompanying it.  If you have received this file from a source
// other than Adobe, then your use, modification, or distribution
// of it requires the prior written permission of Adobe.
//-------------------------------------------------------------------
//-------------------------------------------------------------------------------
//
//	File:
//		DissolveUI.r
//
//	Description:
//		Dialog for the Dissolve Mac project.
//
//-------------------------------------------------------------------------------

#include "Types.r"

resource 'DITL' (16001, "Dissolve UI", purgeable) {
	{	/* array DITLarray: 11 elements */
		/* [1] */
		{8, 220, 28, 288},
		Button {
			enabled,
			"OK"
		},
		/* [2] */
		{40, 220, 60, 288},
		Button {
			enabled,
			"Cancel"
		},
		/* [3] */
		{8, 161, 24, 190},
		EditText {
			enabled,
			""
		},
		/* [4] */
		{8, 77, 24, 153},
		StaticText {
			disabled,
			"Amount %:"
		},
		/* [5] */
		{4, 8, 64, 68},
		UserItem {
			disabled
		},
		/* [6] */
		{40, 72, 56, 153},
		StaticText {
			disabled,
			"Disposition:"
		},
		/* [7] */
		{40, 156, 56, 214},
		RadioButton {
			enabled,
			"Clear"
		},
		/* [8] */
		{56, 156, 72, 214},
		RadioButton {
			enabled,
			"Cool"
		},
		/* [9] */
		{72, 156, 88, 214},
		RadioButton {
			enabled,
			"Hot"
		},
		/* [10] */
		{88, 156, 104, 214},
		RadioButton {
			enabled,
			"Sick"
		},
		/* [11] */
		{88, 8, 104, 112},
		CheckBox {
			enabled,
			"Entire image"
		}
	}
};

resource 'dlgx' (16001) {
	versionZero {
		kDialogFlagsHandleMovableModal + kDialogFlagsUseThemeControls + kDialogFlagsUseThemeBackground
	}
};

resource 'DLOG' (16001, "Dissolve", purgeable) {
	{96, 148, 208, 444},
	movableDBoxProc,
	visible,
	noGoAway,
	0x0,
	16001,
	"Dissolve",
	centerParentWindowScreen
};

// end DissolveUI.r