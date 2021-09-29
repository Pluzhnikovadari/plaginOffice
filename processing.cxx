#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/text/XTextDocument.hpp>
#include <com/sun/star/text/XText.hpp>
#include <com/sun/star/text/XTextTable.hpp>
#include <com/sun/star/text/XTextContent.hpp>
#include <com/sun/star/table/XCell.hpp>
#include <com/sun/star/table/XTable.hpp>

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <cctype>
#include <string>
#include <uchar.h>
#include <iostream>
#include <map>

#include "processing.h"

using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::beans;
using namespace com::sun::star::frame;
using namespace com::sun::star::text;
using namespace com::sun::star::table;
using namespace com::sun::star::util;
using::rtl::OUString;


void Add_text(Reference < XFrame > & rxFrame, rtl::OUString language, int words_count, int word_leght) {
    if (not rxFrame.is())
        return;

    Reference < XComponentLoader > rComponentLoader(rxFrame, UNO_QUERY);
    Reference <XComponent> xWriterComponent = rComponentLoader -> loadComponentFromURL(
            OUString::createFromAscii("private:factory/swriter"),
            OUString::createFromAscii("_blank"),
            0,
            Sequence < ::com::sun::star::beans::PropertyValue > ());
    Reference < XTextDocument > xTextDocument(xWriterComponent, UNO_QUERY);
    Reference < XText > xText = xTextDocument -> getText();
    Reference < XTextCursor > xTextCursor = xText -> createTextCursor();

    rtl::OUString result = "";
    srand(time(NULL));
    for (std::size_t i = 0; i < words_count; i++) {
        rtl::OUString word = "";
        if (language == "latin") {
            std::u16string const charset = u"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

            for (int i = 0; i < word_leght; i++)
                word += (rtl::OUString) charset[rand() % 51];
        } else if (language == "cyrillic") {
            std::u16string const charset = u"абвгдеёжзийклмнопрстуфхцчшщъыьэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ";

            for (int i = 0; i < word_leght; i++)
                word += (rtl::OUString) charset[rand() % 65];
        } else {
            std::u16string const charset = u"абвгдеёжзийклмнопрстуфхцчшщъыьэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

            for (int i = 0; i < word_leght; i++)
                word += (rtl::OUString) charset[rand() % 117];
        }

        result += word + " ";
    }
    xText -> insertString(xTextCursor, result, false);
    return;
}

bool isLetter(rtl::OUString x)
{
    bool flag = false;
    std::u16string const charset = u"абвгдеёжзийклмнопрстуфхцчшщъыьэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    for (int i; i <= 117; i++) {
        if ((rtl::OUString) charset[i] == x) {
            flag = true;
            break;
        }
    }
    return flag;
}

bool isLatin(rtl::OUString x)
{
    bool flag = false;
    std::u16string const charset = u"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    for (int i; i <= 117; i++) {
        if ((rtl::OUString) charset[i] == x) {
            flag = true;
            break;
        }
    }
    return flag;
}


void Statistics(Reference < XFrame > & mxFrame) {
    Reference < XTextDocument > xTextDocument(mxFrame -> getController() -> getModel(), UNO_QUERY);
    Reference < XText > xText = xTextDocument -> getText();
    Reference < XTextCursor > xTextCursor = xText -> createTextCursor();
    Reference < XPropertySet > xCursorProps(xTextCursor, UNO_QUERY);

    std::map <rtl::OUString, int> letters;
    int count = 0;
    bool flag;
    std::u16string const small = u"абвгдеёжзийклмнопрстуфхцчшщъыьэюяabcdefghijklmnopqrstuvwxyz";
    std::u16string const big = u"АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯABCDEFGHIJKLMNOPQRSTUVWXYZ";
    while (xTextCursor -> goRight(1, true)) {
        rtl::OUString elem = xTextCursor -> getString();
        sal_Unicode last_let = (xTextCursor -> getString())[count];

        if (isLetter((OUString) last_let)) {
            for (int k = 0; k < 58; k++) {
                if ((OUString) last_let == (rtl::OUString) big[k]) {
                    last_let = small[k];
                    break;
                }
            }
            flag = false;
            for (auto it = letters.begin(); it != letters.end(); ++it)
            {
                if ((*it).first == (OUString) last_let) {
                    flag = true;
                    it -> second += 1;
                }
            }
            if (flag == false) letters.insert ( std::pair<rtl::OUString, int> (last_let,1) );
        }
        count++;
    }

    xTextCursor -> gotoEnd(false);

    Reference < XMultiServiceFactory > oDocMSF(xTextDocument, UNO_QUERY);
    Reference < XTextTable > xTable(oDocMSF -> createInstance(
            OUString::createFromAscii("com.sun.star.text.TextTable")), UNO_QUERY);


    xTable -> initialize(letters.size() + 1, 2);
    Reference < XTextRange > xTextRange = xText -> getEnd();
    Reference < XTextContent > xTextContent(xTable, UNO_QUERY);
    xText -> insertTextContent(xTextRange, xTextContent, (unsigned char) 0);

    Reference < XCell > xCell = xTable -> getCellByName(OUString::createFromAscii("A1"));
    xText = Reference < XText > (xCell, UNO_QUERY);
    xTextCursor = xText -> createTextCursor();
    xTextCursor -> setString(OUString::createFromAscii("Letters"));

    xCell = xTable -> getCellByName(OUString::createFromAscii("B1"));
    xText = Reference < XText > (xCell, UNO_QUERY);
    xTextCursor = xText -> createTextCursor();
    xTextCursor -> setString(OUString::createFromAscii("Count"));

    int i = 2;
    for (std::map < OUString, int > ::iterator it = letters.begin(); it != letters.end(); ++it) {
        xCell = xTable -> getCellByName(OUString::createFromAscii(((char)('A') + std::to_string(i)).c_str()));
        xText = Reference < XText > (xCell, UNO_QUERY);
        xTextCursor = xText -> createTextCursor();
        xTextCursor -> setString(it -> first);
        xText = Reference < XText > (xCell, UNO_QUERY);
        xTextCursor = xText -> createTextCursor();
        xCell = xTable -> getCellByName(OUString::createFromAscii(((char)('B') + std::to_string(i)).c_str()));
        xCell -> setValue(it -> second);
        ++i;
    }

    return;
}


void Red(Reference < XFrame > & mxFrame) {
    Reference < XTextDocument > xTextDocument(mxFrame -> getController() -> getModel(), UNO_QUERY);
    Reference < XText > xText = xTextDocument -> getText();
    Reference < XTextCursor > xTextCursor = xText -> createTextCursor();
    Reference < XPropertySet > xCursorProps(xTextCursor, UNO_QUERY);

    bool flag = false;
    int count = 0;
    int size, prev_len = 0;
    bool table = false;

    while (xTextCursor -> goRight(1, true)) {
        rtl::OUString elem = xTextCursor -> getString();
        size = elem.getLength();
        sal_Unicode last_let = (xTextCursor -> getString())[size - 1];

        if (isLetter((OUString)last_let)) {
            if (isLatin((OUString)last_let)) flag = true;

        }
        else {
            if (flag) {
                xTextCursor -> goLeft(1, true);
                xCursorProps -> setPropertyValue("CharBackColor", makeAny(0xFF0000));
                xTextCursor -> goRight(1, true);

            }
            flag = false;
            prev_len = 0;
            xTextCursor -> collapseToEnd();
        }
        prev_len++;
    }
    if (flag) {
        if (size - prev_len > 1) {
            xTextCursor -> goLeft(1, true);
            xCursorProps -> setPropertyValue("CharBackColor", makeAny(0xFF0000));
            xTextCursor -> goRight(1, true);
        } else {
            xCursorProps->setPropertyValue("CharBackColor", makeAny(0xFF0000));
        }
    }

    return;
}