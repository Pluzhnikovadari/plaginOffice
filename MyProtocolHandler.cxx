/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */


#include "ListenerHelper.h"
#include "MyProtocolHandler.h"
#include "processing.h"

#include <com/sun/star/awt/MessageBoxButtons.hpp>
#include <com/sun/star/awt/Toolkit.hpp>
#include <com/sun/star/awt/XMessageBoxFactory.hpp>
#include <com/sun/star/frame/ControlCommand.hpp>
#include <com/sun/star/text/XTextViewCursorSupplier.hpp>
#include <com/sun/star/sheet/XSpreadsheetView.hpp>
#include <com/sun/star/system/SystemShellExecute.hpp>
#include <com/sun/star/system/SystemShellExecuteFlags.hpp>
#include <com/sun/star/system/XSystemShellExecute.hpp>
#include <cppuhelper/supportsservice.hxx>


using namespace com::sun::star::awt;
using namespace com::sun::star::frame;
using namespace com::sun::star::system;
using namespace com::sun::star::uno;

using com::sun::star::beans::NamedValue;
using com::sun::star::beans::PropertyValue;
using com::sun::star::sheet::XSpreadsheetView;
using com::sun::star::text::XTextViewCursorSupplier;
using com::sun::star::util::URL;

ListenerHelper aListenerHelper;

void BaseDispatch::ShowMessageBox( const Reference< XFrame >& rFrame, const ::rtl::OUString& aTitle, const ::rtl::OUString& aMsgText )
{
    if ( !mxToolkit.is() )
        mxToolkit = Toolkit::create(mxContext);
    Reference< XMessageBoxFactory > xMsgBoxFactory( mxToolkit, UNO_QUERY );
    if ( rFrame.is() && xMsgBoxFactory.is() )
    {
        Reference< XMessageBox > xMsgBox = xMsgBoxFactory->createMessageBox(
            Reference< XWindowPeer >( rFrame->getContainerWindow(), UNO_QUERY ),
            com::sun::star::awt::MessageBoxType_INFOBOX,
            MessageBoxButtons::BUTTONS_OK,
            aTitle,
            aMsgText );

        if ( xMsgBox.is() )
            xMsgBox->execute();
    }
}

void BaseDispatch::SendCommand( const com::sun::star::util::URL& aURL, const ::rtl::OUString& rCommand, const Sequence< NamedValue >& rArgs, sal_Bool bEnabled )
{
    Reference < XDispatch > xDispatch =
            aListenerHelper.GetDispatch( mxFrame, aURL.Path );

    FeatureStateEvent aEvent;

    aEvent.FeatureURL = aURL;
    aEvent.Source     = xDispatch;
    aEvent.IsEnabled  = bEnabled;
    aEvent.Requery    = sal_False;

    ControlCommand aCtrlCmd;
    aCtrlCmd.Command   = rCommand;
    aCtrlCmd.Arguments = rArgs;

    aEvent.State <<= aCtrlCmd;
    aListenerHelper.Notify( mxFrame, aEvent.FeatureURL.Path, aEvent );
}

void BaseDispatch::SendCommandTo( const Reference< XStatusListener >& xControl, const URL& aURL, const ::rtl::OUString& rCommand, const Sequence< NamedValue >& rArgs, sal_Bool bEnabled )
{
    FeatureStateEvent aEvent;

    aEvent.FeatureURL = aURL;
    aEvent.Source     = (::com::sun::star::frame::XDispatch*) this;
    aEvent.IsEnabled  = bEnabled;
    aEvent.Requery    = sal_False;

    ControlCommand aCtrlCmd;
    aCtrlCmd.Command   = rCommand;
    aCtrlCmd.Arguments = rArgs;

    aEvent.State <<= aCtrlCmd;
    xControl->statusChanged( aEvent );
}

void SAL_CALL MyProtocolHandler::initialize( const Sequence< Any >& aArguments )
{
    Reference < XFrame > xFrame;
    if ( aArguments.getLength() )
    {
        // the first Argument is always the Frame, as a ProtocolHandler needs to have access
        // to the context in which it is invoked.
        aArguments[0] >>= xFrame;
        mxFrame = xFrame;
    }
}

Reference< XDispatch > SAL_CALL MyProtocolHandler::queryDispatch(   const URL& aURL, const ::rtl::OUString& sTargetFrameName, sal_Int32 nSearchFlags )
{
    Reference < XDispatch > xRet;
    if ( !mxFrame.is() )
        return 0;

    Reference < XController > xCtrl = mxFrame->getController();
    if ( xCtrl.is() && aURL.Protocol == "vnd.demo.complextoolbarcontrols.demoaddon:" )
    {
        Reference < XTextViewCursorSupplier > xCursor( xCtrl, UNO_QUERY );
        Reference < XSpreadsheetView > xView( xCtrl, UNO_QUERY );
        if ( !xCursor.is() && !xView.is() )
            // without an appropriate corresponding document the handler doesn't function
            return xRet;

        if ( aURL.Path == "Words_count" ||
             aURL.Path == "Layout_choosing" ||
             aURL.Path == "Max_lenght" ||
             aURL.Path == "Gen_text" ||
             aURL.Path == "Statistics" ||
             aURL.Path == "Red" )
        {
            xRet = aListenerHelper.GetDispatch( mxFrame, aURL.Path );
            if ( !xRet.is() )
            {
                xRet = xCursor.is() ? (BaseDispatch*) new WriterDispatch( mxContext, mxFrame ) :
                    (BaseDispatch*) new CalcDispatch( mxContext, mxFrame );
                aListenerHelper.AddDispatch( xRet, mxFrame, aURL.Path );
            }
        }
    }

    return xRet;
}

Sequence < Reference< XDispatch > > SAL_CALL MyProtocolHandler::queryDispatches( const Sequence < DispatchDescriptor >& seqDescripts )
{
    sal_Int32 nCount = seqDescripts.getLength();
    Sequence < Reference < XDispatch > > lDispatcher( nCount );

    for( sal_Int32 i=0; i<nCount; ++i )
        lDispatcher[i] = queryDispatch( seqDescripts[i].FeatureURL, seqDescripts[i].FrameName, seqDescripts[i].SearchFlags );

    return lDispatcher;
}

::rtl::OUString MyProtocolHandler_getImplementationName ()
{
    return ::rtl::OUString( MYPROTOCOLHANDLER_IMPLEMENTATIONNAME );
}

Sequence< ::rtl::OUString > SAL_CALL MyProtocolHandler_getSupportedServiceNames(  )
{
    Sequence < ::rtl::OUString > aRet(1);
    aRet[0] = ::rtl::OUString( MYPROTOCOLHANDLER_SERVICENAME );
    return aRet;
}

#undef SERVICE_NAME

Reference< XInterface > SAL_CALL MyProtocolHandler_createInstance( const Reference< XComponentContext > & rSMgr)
{
    return (cppu::OWeakObject*) new MyProtocolHandler( rSMgr );
}

// XServiceInfo
::rtl::OUString SAL_CALL MyProtocolHandler::getImplementationName(  )
{
    return MyProtocolHandler_getImplementationName();
}

sal_Bool SAL_CALL MyProtocolHandler::supportsService( const ::rtl::OUString& rServiceName )
{
    return cppu::supportsService(this, rServiceName);
}

Sequence< ::rtl::OUString > SAL_CALL MyProtocolHandler::getSupportedServiceNames(  )
{
    return MyProtocolHandler_getSupportedServiceNames();
}


int WORDS_COUNT = 0;
rtl::OUString LANGUAGE;
int WORD_LEN;
rtl::OUString lenght;


void SAL_CALL BaseDispatch::dispatch( const URL& aURL, const Sequence < PropertyValue >& lArgs )
{
    /* It's necessary to hold this object alive, till this method finishes.
       May the outside dispatch cache (implemented by the menu/toolbar!)
       forget this instance during de-/activation of frames (focus!).

        E.g. An open db beamer in combination with the My-Dialog
        can force such strange situation :-(
     */
    Reference< XInterface > xSelfHold(static_cast< XDispatch* >(this), UNO_QUERY);

    if ( aURL.Protocol == "vnd.demo.complextoolbarcontrols.demoaddon:" )
    {
        if ( aURL.Path == "Words_count" )
        {
            int words_count = 0;
            for (sal_Int32 i = 0; i < lArgs.getLength(); i++)
            {
                if (lArgs[i].Name == "Value")
                {
                    lArgs[i].Value >>= words_count;
                    break;
                }
            }
            WORDS_COUNT = words_count;
        }

        else if ( aURL.Path == "Layout_choosing" )
        {
            rtl::OUString aText;
            for ( sal_Int32 i = 0; i < lArgs.getLength(); i++ )
            {
                if ( lArgs[i].Name == "Text" )
                {
                    lArgs[i].Value >>= aText;
                    break;
                }
            }

            LANGUAGE = aText;
        }

        else if ( aURL.Path == "Max_lenght" )
        {
            rtl::OUString aText;
            for ( sal_Int32 i = 0; i < lArgs.getLength(); i++ )
            {
                if ( lArgs[i].Name == "Text" )
                {

                    lArgs[i].Value >>= aText;
                    break;
                }
            }
            lenght = aText;
            int L = lenght.getLength();
            bool flag = 1;
            for(sal_Int32 i = 0; i < L; i++)
                if(lenght[i]<'0' || lenght[i]>'9') {
                    flag = 0;
                    break;
                }

            if (flag) WORD_LEN = aText.toInt32();
            else ShowMessageBox(mxFrame, rtl::OUString("Error"), rtl::OUString("Wrong arguments"));
        }


        else if ( aURL.Path == "Gen_text" )
        {
            if (WORDS_COUNT <= 0 or WORD_LEN <= 0 or LANGUAGE.getLength() <= 0) {
                ShowMessageBox(mxFrame, rtl::OUString("Error"), rtl::OUString("Wrong arguments"));
            }
            else {

                Add_text(mxFrame, LANGUAGE, WORDS_COUNT, WORD_LEN);
                LANGUAGE = "";
                WORDS_COUNT = 0;
                WORD_LEN = 0;
            }
        }
        else if ( aURL.Path == "Statistics" )
        {
            Statistics(mxFrame);
        }
        else if ( aURL.Path == "Red" )
        {
            Red(mxFrame);
        }
    }
}

void SAL_CALL BaseDispatch::addStatusListener( const Reference< XStatusListener >& xControl, const URL& aURL )
{
    if ( aURL.Protocol == "vnd.demo.complextoolbarcontrols.demoaddon:" )
    {
        if (aURL.Path == "Words_count") {
            // A spin button
            Sequence < NamedValue > aArgs(4);

            // send command to initialize spin button
            aArgs[0].Name = "Value";
            aArgs[0].Value <<= int(0);
            aArgs[1].Name = "LowerLimit";
            aArgs[1].Value <<= int(0);
            aArgs[2].Name = "Step";
            aArgs[2].Value <<= int(1);
            aArgs[3].Name = "OutputFormat";
            aArgs[3].Value <<= rtl::OUString("%d");

            SendCommandTo(xControl, aURL, rtl::OUString("SetValues"), aArgs, sal_True);
        }

        else if ( aURL.Path == "Layout_choosing" )
        {
            // A toggle dropdown box is normally used for a group of commands
            // where the user can select the last issued command easily.
            // E.g. a typical command group would be "Insert shape"
            Sequence< NamedValue > aArgs( 1 );

            // send command to set context menu content
            Sequence< rtl::OUString > aContextMenu( 3 );
            aContextMenu[0] = "cyrillic";
            aContextMenu[1] = "latin";
            aContextMenu[2] = "mixture";

            aArgs[0].Name = "List";
            aArgs[0].Value <<= aContextMenu;
            SendCommandTo( xControl, aURL, rtl::OUString( "SetList" ), aArgs, sal_True );

            // send command to check item on pos=0
            aArgs[0].Name = rtl::OUString( "Pos" );
            aArgs[0].Value <<= sal_Int32( 0 );
            SendCommandTo( xControl, aURL, ::rtl::OUString( "CheckItemPos" ), aArgs, sal_True );
        }

        aListenerHelper.AddListener( mxFrame, xControl, aURL.Path );
    }
}

void SAL_CALL BaseDispatch::removeStatusListener( const Reference< XStatusListener >& xControl, const URL& aURL )
{
    aListenerHelper.RemoveListener( mxFrame, xControl, aURL.Path );
}

void SAL_CALL BaseDispatch::controlEvent( const ControlEvent& Event )
{
    if ( Event.aURL.Protocol == "vnd.demo.complextoolbarcontrols.demoaddon:" )
    {
        if ( Event.aURL.Path == "ComboboxCmd" )
        {
            // We get notifications whenever the text inside the combobox has been changed.
            // We store the new text into a member.
            if ( Event.Event == "TextChanged" )
            {
                rtl::OUString aNewText;
                sal_Bool      bHasText( sal_False );
                for ( sal_Int32 i = 0; i < Event.aInformation.getLength(); i++ )
                {
                    if ( Event.aInformation[i].Name == "Text" )
                    {
                        bHasText = Event.aInformation[i].Value >>= aNewText;
                        break;
                    }
                }

                if ( bHasText )
                    maComboBoxText = aNewText;
            }
        }
    }
}

BaseDispatch::BaseDispatch( const Reference< XComponentContext > &rxContext,
                            const Reference< XFrame >& xFrame,
                            const ::rtl::OUString& rServiceName )
        : mxContext( rxContext )
        , mxFrame( xFrame )
        , msDocService( rServiceName )
        , mbButtonEnabled( sal_True )
{
}

BaseDispatch::~BaseDispatch()
{
    mxFrame.clear();
    mxContext.clear();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
