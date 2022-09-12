/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2022 Open Ephys

------------------------------------------------------------------

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "LeadORPluginEditor.h"

LeadORPluginEditor::LeadORPluginEditor(GenericProcessor *parentNode)
    : GenericEditor(parentNode)
{

    desiredWidth = 200;

    addCheckBoxParameterEditor("Feature", 10, 22);
    addTextBoxParameterEditor("Feature_Name", 90, 22);
    addCheckBoxParameterEditor("Spikes", 10, 62);
    addSelectedChannelsParameterEditor("Channels", 10, 108);

    // add igtlink button
    igtLinkButton = new UtilityButton("IGTLink", Font("Small Text", 13, Font::plain));
    igtLinkButton->setRadius(3.0f);
    igtLinkButton->setBounds(100, 108, 80, 20);
    igtLinkButton->addListener(this);
    igtLinkButton->setTooltip("IGTLink connection");
    addAndMakeVisible(igtLinkButton);
}

void LeadORPluginEditor::buttonClicked(Button *button)
{
    if (button == igtLinkButton)
    {
        LeadORPlugin *processor = (LeadORPlugin *)getProcessor();
        auto *igtlConnection = new IGTLConnectionPopUp(processor);

        CallOutBox &myBox = CallOutBox::launchAsynchronously(std::unique_ptr<Component>(igtlConnection),
                                                             button->getScreenBounds(),
                                                             nullptr);
    }
}

IGTLConnectionPopUp::IGTLConnectionPopUp(LeadORPlugin *processor)
{
    leadORProcessor = processor;
    bool connected = leadORProcessor->openIGTLinkLogic->isConnected();

    portEditor = new TextEditor("Port", 0);
    portEditor->setBounds(10, 3, 80, 20);
    portEditor->setTooltip("");
    portEditor->setEnabled(connected ? false : true);
    portEditor->setText(String(leadORProcessor->openIGTLinkLogic->currentPort));
    addAndMakeVisible(portEditor);

    connectButton = new UtilityButton(connected ? "Disconnect" : "Connect", Font("Small Text", 13, Font::plain));
    connectButton->setRadius(3.0f);
    connectButton->setBounds(10, 28, 80, 20);
    connectButton->addListener(this);
    connectButton->setTooltip("");
    addAndMakeVisible(connectButton);

    setSize(100, 50);
    setColour(ColourSelector::backgroundColourId, Colours::transparentBlack);
}

void IGTLConnectionPopUp::buttonClicked(Button *button)
{
    if (button == connectButton)
    {
        bool connected = false;
        if (connectButton->getLabel().equalsIgnoreCase("Connect"))
        {
            connected = leadORProcessor->openIGTLinkLogic->startIGTLinkConnection(portEditor->getText().getIntValue());
            if (connected)
                leadORProcessor->openIGTLinkLogic->currentPort = portEditor->getText().getIntValue();
        }
        else
        {
            leadORProcessor->openIGTLinkLogic->closeConnection();
        }
        connectButton->setLabel(connected ? "Disconnect" : "Connect");
        portEditor->setEnabled(connected ? false : true);
    }
}