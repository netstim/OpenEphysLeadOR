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

int LeadORPlugin::LeadORPlugInID = 0;
int LeadORPlugin::RecordingSiteID = 0;
bool LeadORPlugin::InitialMsgSent = false;

LeadORPlugin::LeadORPlugin()
    : GenericProcessor("Lead-OR")
{
    addBooleanParameter(Parameter::GLOBAL_SCOPE, "Spikes", "Send Spikes through igtlink", false, false);
    addBooleanParameter(Parameter::GLOBAL_SCOPE, "Feature", "Send stream data through igtlink", false, false);
    addStringParameter(Parameter::GLOBAL_SCOPE, "Feature_Name", "Name assigned to the feature", "Feature Name", true);
    addSelectedChannelsParameter(Parameter::STREAM_SCOPE, "Channels", "The input channels to analyze");

    previous_ms = Time::currentTimeMillis(); // TODO: when strat aquisition
    OpenIGTLinkCommon *openIGTLinkLogic = new OpenIGTLinkCommon();

    leadORPlugInID = LeadORPlugin::LeadORPlugInID++;
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

    if (LeadORPlugin::RecordingSiteID < 1)
        return;

    for (auto stream : getDataStreams())
    {
        if ((*stream)["enable_stream"])
        {
            const uint16 streamId = stream->getStreamId();
            const uint32 nSamples = getNumSamplesInBlock(streamId);

            int currentRecordingSiteIdx = (RecordingSites.size() - 1) * (NumChannels + 1);

            FeatureValues.set(currentRecordingSiteIdx, LeadORPlugin::RecordingSiteID);

            for (auto chan : *((*stream)["Channels"].getArray()))
            {
                FeatureValues.set(currentRecordingSiteIdx + (int)chan + 1, buffer.getSample(chan, 0));
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
        if (leadORPlugInID > 0)
            return;

        StringArray messageParts;
        messageParts.addTokens(message, ":", "\"");
        if (messageParts.size() != 3)
            return;

        if (messageParts[1].equalsIgnoreCase("DistanceToTarget"))
        {
            float distanceToTarget = messageParts[2].getFloatValue();
            sendDistanceToTargetMsg(distanceToTarget);

            if (timeElapsedSinceLastIsStable())
            {
                sendRecordingSitesMsg();
                LeadORPlugin::RecordingSiteID++;
                RecordingSites.add(distanceToTarget);
                broadcastMessage("LeadOR:SendFeatureData");
            }
        }
    }
    else if (message.equalsIgnoreCase("LeadOR:SendFeatureData"))
    {
        if (getParameter("Feature")->getValue())
            sendFeatureValuesMsg();
        FeatureValues.insertMultiple(RecordingSites.size() * (NumChannels + 1), 0.0, NumChannels + 1);
    }
}

bool LeadORPlugin::timeElapsedSinceLastIsStable()
{
    int64 current_ms = Time::currentTimeMillis();
    bool isStable = (current_ms - previous_ms) > 4000;
    previous_ms = current_ms;
    return isStable;
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

void LeadORPlugin::sendRecordingSitesMsg()
{
    if (RecordingSites.size() < 1)
        return;

    Array<float> *values = new Array<float>();

    for (int i = 0; i < RecordingSites.size(); i++)
    {
        values->add(RecordingSites[i]);
        values->add(0);
        values->add(0);
    }

    openIGTLinkLogic->sendPointMessage("LeadOR:RecordingSite", *values);
}

void LeadORPlugin::sendFeatureValuesMsg()
{
    String msg = "RecordingSiteID";
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

    openIGTLinkLogic->sendStringMessage(String("LeadOR:") + getParameter("Feature_Name")->getValue(), msg);
}
void LeadORPlugin::saveCustomParametersToXml(XmlElement *parentElement)
{
}

void LeadORPlugin::loadCustomParametersFromXml(XmlElement *parentElement)
{
}
