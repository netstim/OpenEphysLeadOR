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

LeadORPlugin::LeadORPlugin()
    : GenericProcessor("Lead-OR")
{
    prev_ms = Time::currentTimeMillis(); // TODO: when strat aquisition
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
}

void LeadORPlugin::process(AudioBuffer<float> &buffer)
{

    checkForEvents(true);
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
            String msg = "IGTL:Transform:DistanceToTarget:1:0:0:0:0:1:0:0:0:0:1:" + messageParts[2].toStdString();
            int64 curr_ms = Time::currentTimeMillis();
            if ((curr_ms - prev_ms) > 4000)
            {
                DTTArray.add(messageParts[2]);
                msg += ";" + getRecordingSitesMsg();
            }
            prev_ms = curr_ms;
            broadcastMessage(msg);
        }
    }
}

String LeadORPlugin::getRecordingSitesMsg()
{
    String msg = "IGTL:Point:RecordingSites";
    for (int i = 0; i < DTTArray.size(); i++)
    {
        msg += ":0,0," + DTTArray[i] + ",name";
    }
    return msg;
}

void LeadORPlugin::saveCustomParametersToXml(XmlElement *parentElement)
{
}

void LeadORPlugin::loadCustomParametersFromXml(XmlElement *parentElement)
{
}
