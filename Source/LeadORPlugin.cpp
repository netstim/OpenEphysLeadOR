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

#include "LeadORPlugin.h"

#include "LeadORPluginEditor.h"

#include <OpenIGTLinkCommon.h>

LeadORPlugin::LeadORPlugin()
    : GenericProcessor("Lead-OR")
{

    addSelectedChannelsParameter(Parameter::STREAM_SCOPE,
                                 "Channels", "The input channels to analyze");

    prev_ms = Time::currentTimeMillis(); // TODO: when strat aquisition
    OpenIGTLinkCommon *openIGTLinkLogic = new OpenIGTLinkCommon();
}

LeadORPlugin::~LeadORPlugin()
{
}

AudioProcessorEditor *LeadORPlugin::createEditor()
{
    editor = std::make_unique<LeadORPluginEditor>(this);
    return editor.get();
}

void LeadORPlugin::updateSettings()
{
    numChannels = getNumInputs();
    channelsNamesArray.clear();
    valuesArray.clear();

    for (auto stream : getDataStreams())
    {
        if ((*stream)["enable_stream"])
        {
            for (auto chan : *((*stream)["Channels"].getArray()))
            {
                channelsNamesArray.add(getContinuousChannel(int(chan))->getName());
            }
        }
    }
    openIGTLinkLogic->sendStringMessage("LeadOR:ChannelsNames", channelsNamesArray.joinIntoString(","));
}

void LeadORPlugin::process(AudioBuffer<float> &buffer)
{
    checkForEvents(true);

    if (!initMsgSent)
        sendInitMsg();

    if (recordingSiteID < 1)
        return;

    for (auto stream : getDataStreams())
    {
        if ((*stream)["enable_stream"])
        {
            const uint16 streamId = stream->getStreamId();
            const uint32 nSamples = getNumSamplesInBlock(streamId);

            valuesArray.set((DTTArray.size() - 1) * (numChannels + 1), recordingSiteID);

            for (auto chan : *((*stream)["Channels"].getArray()))
            {
                valuesArray.set((DTTArray.size() - 1) * (numChannels + 1) + (int)chan + 1, buffer.getSample(chan, 0));
            }
        }
    }
}

void LeadORPlugin::handleTTLEvent(TTLEventPtr event)
{
}

void LeadORPlugin::handleSpike(SpikePtr event)
{
}

void LeadORPlugin::handleBroadcastMessage(String message)
{
    if (message.startsWith("MicroDrive"))
    {
        StringArray messageParts;
        messageParts.addTokens(message, ":", "\"");
        if (messageParts.size() != 3)
            return;

        if (messageParts[1].equalsIgnoreCase("DistanceToTarget"))
        {
            Array<float> *values = new Array<float>(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, messageParts[2].getFloatValue());
            openIGTLinkLogic->sendTransformMessage("LeadOR:DTT", *values);

            int64 curr_ms = Time::currentTimeMillis();
            if ((curr_ms - prev_ms) > 4000)
            {
                if (DTTArray.size() > 0)
                {
                    sendRecordingSitesMsg();
                    sendChannelsValuesMsg();
                }
                recordingSiteID++;
                DTTArray.add(messageParts[2]);
                valuesArray.insertMultiple(DTTArray.size() * (numChannels + 1), 0.0, numChannels);
            }
            prev_ms = curr_ms;
        }
    }
}

void LeadORPlugin::sendInitMsg()
{
    openIGTLinkLogic->sendStringMessage("LeadOR:ChannelsNames", channelsNamesArray.joinIntoString(","));
    Array<float> *values = new Array<float>(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 30);
    openIGTLinkLogic->sendTransformMessage("LeadOR:DTT", *values);
    initMsgSent = true;
}

void LeadORPlugin::sendRecordingSitesMsg()
{
    Array<float> *values = new Array<float>();

    for (int i = 0; i < DTTArray.size(); i++)
    {
        values->add(DTTArray[i].getFloatValue());
        values->add(0);
        values->add(0);
    }

    openIGTLinkLogic->sendPointMessage("LeadOR:RecordingSite", *values);
}

void LeadORPlugin::sendChannelsValuesMsg()
{
    String msg = "RecordingSiteID";
    for (int chan = 0; chan < numChannels; chan++)
        msg += "," + channelsNamesArray[chan];
    msg += "\n";

    for (int i = 0; i < valuesArray.size(); i++)
    {
        if (i == 0)
            msg += "";
        else if (i % (numChannels + 1) == 0)
            msg += "\n";
        else
            msg += ",";
        msg += String(valuesArray[i], 3, false);
    }

    openIGTLinkLogic->sendStringMessage("LeadOR:NRMS", msg);
}
void LeadORPlugin::saveCustomParametersToXml(XmlElement *parentElement)
{
}

void LeadORPlugin::loadCustomParametersFromXml(XmlElement *parentElement)
{
}
