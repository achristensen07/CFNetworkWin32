/*
 * Copyright (c) 1997-2004 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@

    Change History (most recent first):
    
$Log: FourthPage.h,v $
Revision 1.3  2005/07/07 17:53:20  shersche
Fix problems associated with the CUPS printer workaround fix.

Revision 1.2  2005/01/06 08:17:08  shersche
Display the selected protocol ("Raw", "LPR", "IPP") rather than the port name

Revision 1.1  2004/06/18 04:36:57  rpantos
First checked in


*/

#pragma once
#include "afxwin.h"


// CFourthPage dialog

class CFourthPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CFourthPage)

public:
	CFourthPage();
	virtual ~CFourthPage();

// Dialog Data
	enum { IDD = IDD_FOURTH_PAGE };

	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:

	OSStatus	OnInitPage();
	CFont		m_largeFont;
	bool		m_initialized;


public:
	CStatic m_goodbye;
private:
	CStatic m_printerNameCtrl;
	CStatic m_printerManufacturerCtrl;
	CStatic m_printerModelCtrl;
	CStatic m_printerProtocolCtrl;
	CStatic m_printerDefault;
};
