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

#include <OpenEphysIGTLink.h>

bool LeadORPlugin::InitialMsgSent = false;

LeadORPlugin::LeadORPlugin()
    : GenericProcessor("Lead-OR")
{
    // addBooleanParameter(Parameter::GLOBAL_SCOPE, "Spikes", "Send Spikes through igtlink", false, false);
    addBooleanParameter(Parameter::GLOBAL_SCOPE, "Feature", "Send stream data through igtlink", false, false);
    addStringParameter(Parameter::GLOBAL_SCOPE, "Feature_Name", "Name assigned to the feature", "Feature Name", true);
    addStringParameter(Parameter::GLOBAL_SCOPE, "Distance_To_Target", "Micro Drive distance to target", "Distance To Target", true);
    addSelectedChannelsParameter(Parameter::STREAM_SCOPE, "Channels", "The input channels to analyze");

    OpenIGTLinkLogic *openIGTLinkLogic = new OpenIGTLinkLogic();
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
    NumChannels = getNumInputs();
    FeatureValues.clear();
    updateChannelsNames();
    sendChannelsMsg();
}

void LeadORPlugin::updateChannelsNames()
{
    ChannelsNamesArray.clear();
    for (auto stream : getDataStreams())
    {
        if ((*stream)["enable_stream"])
        {
            for (auto chan : *((*stream)["Channels"].getArray()))
            {
                ChannelsNamesArray.add(getContinuousChannel(int(chan))->getName());
            }
        }
    }
}

void LeadORPlugin::process(AudioBuffer<float> &buffer)
{
    checkForEvents(true);

    if (!LeadORPlugin::InitialMsgSent)
        sendInitMsg();

    if (FeatureValues.isEmpty())
        return;

    for (auto stream : getDataStreams())
    {
        if ((*stream)["enable_stream"])
        {
            const uint16 streamId = stream->getStreamId();
            const uint32 nSamples = getNumSamplesInBlock(streamId);

            int currentRecordingSiteIdx = FeatureValues.size() - (NumChannels + 1);

            FeatureValues.set(currentRecordingSiteIdx, DistanceToTarget);

            for (auto chan : *((*stream)["Channels"].getArray()))
            {
                FeatureValues.set(currentRecordingSiteIdx + (int)chan + 1, buffer.getSample(chan, 0));
            }
        }
    }

    int64 current_feature_ms = Time::currentTimeMillis();
    if (getParameter("Feature")->getValue() && ((current_feature_ms - previous_feature_ms) > 1000))
    {
        sendFeatureValuesMsg();
        previous_feature_ms = current_feature_ms;
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
            DistanceToTarget = messageParts[2].getFloatValue();
            sendDistanceToTargetMsg(DistanceToTarget);
            getParameter("Distance_To_Target")->setNextValue(messageParts[2]);

            if (!FeatureValues.isEmpty() && getParameter("Feature")->getValue())
                sendFeatureValuesMsg();

            int64 current_dtt_ms = Time::currentTimeMillis();
            if ((FeatureValues.isEmpty()) || ((current_dtt_ms - previous_dtt_ms) > 4000))
            {
                for (int i = 0; i < NumChannels + 1; i++)
                    FeatureValues.add(0.0);
            }
            previous_dtt_ms = current_dtt_ms;
        }
    }
}

String LeadORPlugin::handleConfigMessage(String message)
{
    // Available commands:
    // LOR IGTLCONNECT <port>
    // LOR IGTLDISCONNECT
    StringArray messageParts = StringArray::fromTokens(message, " ", "");
    if (messageParts[0].equalsIgnoreCase("LOR") && (messageParts.size() >= 1))
    {
        String command = messageParts[1];
        if (command.equalsIgnoreCase("IGTLCONNECT") && (messageParts.size() == 3))
        {
            int port = messageParts[2].getIntValue();
            bool connected = openIGTLinkLogic->startIGTLinkConnection(port);
            return (connected ? "Connected!" : "Unable to connect.");
        }
        else if (command.equalsIgnoreCase("IGTLDISCONNECT"))
        {
            openIGTLinkLogic->closeConnection();
        }
        else
        {
            return "Lead-OR unrecognized command: " + command + ".";
        }
    }
    return "Command not recognized.";
}

void LeadORPlugin::sendInitMsg()
{
    sendChannelsMsg();
    sendDistanceToTargetMsg(30);
    LeadORPlugin::InitialMsgSent = true;
}

void LeadORPlugin::sendChannelsMsg()
{
    openIGTLinkLogic->sendStringMessage("LeadOR:ChannelsNames", ChannelsNamesArray.joinIntoString(","));
}

void LeadORPlugin::sendDistanceToTargetMsg(float distanceToTarget)
{
    Array<float> *values = new Array<float>(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, distanceToTarget);
    openIGTLinkLogic->sendTransformMessage("LeadOR:DTT", *values);
}

void LeadORPlugin::sendFeatureValuesMsg()
{
    String msg = "RecordingSiteDTT";
    for (int chan = 0; chan < NumChannels; chan++)
        msg += "," + ChannelsNamesArray[chan];
    msg += "\n";

    for (int i = 0; i < FeatureValues.size(); i++)
    {
        if (i == 0)
            msg += "";
        else if (i % (NumChannels + 1) == 0)
            msg += "\n";
        else
            msg += ",";
        msg += String(FeatureValues[i], 3, false);
    }

    openIGTLinkLogic->sendStringMessage(String("LeadOR:") + getParameter("Feature_Name")->getValueAsString(), msg);
}
void LeadORPlugin::saveCustomParametersToXml(XmlElement *parentElement)
{
}

void LeadORPlugin::loadCustomParametersFromXml(XmlElement *parentElement)
{
}
