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

#ifndef PROCESSORPLUGINEDITOR_H_DEFINED
#define PROCESSORPLUGINEDITOR_H_DEFINED

#include <EditorHeaders.h>

#include "LeadORPlugin.h"

class LeadORPluginEditor : public GenericEditor,
						   public Button::Listener
{
public:
	/** Constructor */
	LeadORPluginEditor(GenericProcessor *parentNode);

	/** Destructor */
	~LeadORPluginEditor() {}

	/** Respond to button clicks*/
	void buttonClicked(Button *button);

private:
	ScopedPointer<UtilityButton> igtLinkButton;
	/** Generates an assertion if this class leaks */
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LeadORPluginEditor);
};

class MySelectedChannelsParameterEditor : public SelectedChannelsParameterEditor
{
public:
	/** Constructor */
	MySelectedChannelsParameterEditor(Parameter *param, LeadORPlugin *processor) : SelectedChannelsParameterEditor(param)
	{
		p = processor;
	}

	/** Destructor */
	virtual ~MySelectedChannelsParameterEditor() {}

	/** Responds to changes in the PopupChannelSelector*/
	void channelStateChanged(Array<int> selectedChannels)
	{
		SelectedChannelsParameterEditor::channelStateChanged(selectedChannels);
		p->updateSettings();
	};

private:
	LeadORPlugin *p;
};

class IGTLConnectionPopUp : public Component,
							public Button::Listener
{
public:
	/** Constructor */
	IGTLConnectionPopUp(LeadORPlugin *processor);

	/** Destructor */
	~IGTLConnectionPopUp() {}

	/** Respond to button clicks*/
	void buttonClicked(Button *button);

private:
	LeadORPlugin *leadORProcessor;
	ScopedPointer<UtilityButton> connectButton;
	ScopedPointer<TextEditor> portEditor;
};

#endif // PROCESSORPLUGINEDITOR_H_DEFINED