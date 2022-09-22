*This project is work in progress and does not have any regulatory approval.*
# Open Ephys Lead-OR

Plugin for the Open Ephys GUI to interface between micro electrode recording devices (for example the NeuroOmega, via the [OpenEphysNeuroOmega plugin](https://github.com/netstim/OpenEphysNeuroOmega)) and the visualization component of Lead-OR via the [3DSlicer extension](https://github.com/netstim/SlicerNetstim).

## Installation

The plugin depends on the [OpenEphysIGTLink](https://github.com/netstim/OpenEphysIGTLink) library. The library must be installed to use this plugin.

- Manual

The compiled dll for GUI v6 is available from the Releases page. It should be downloaded and placed under `C:\ProgramData\Open Ephys\plugins-api8`.

- Github CLI

Using Github CLI is easy to stay up to date with latest release using the following command:

```PowerShell
gh release download --clobber --dir "C:\ProgramData\Open Ephys\plugins-api8" --pattern *.dll --repo netstim/OpenEphysLeadOR
```

- From Source

Alternativly, one can also compile this plugin from source. See Open Ephys GUI Documentation for instructions.

- From the GUI

The plugin is currently not available from the GUI Plugin installer. Use one of the avobe methods.