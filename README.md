## Building the UMCPSEG Plugin

To build the University of Michigan CPSEG (UMCPSEG) plugin you must integrate 
the UMCPSEG source code into your local PluginSDK installation. These 
instructions assume you have already downloaded, installed, and 
[verified][SDKdocs] the [Pointwise Plugin SDK][SDKdownload]. Be sure the SDK you 
are using is compatible with the version of Pointwise you are targeting.

To integrate the github UMCPSEG source code, you must first create a plugin 
project in your SDK using the steps shown below. You may replace `MyUMCPSEG` 
with any unique name you choose. The UMCPSEG plugin uses the C++ API. 
Consequently, you *must* use the `-cpp` option with `mkplugin` below.

If you plan on building the plugin for multiple platforms, I suggest that you 
place the SDK on a network drive. That way you can build the UMCPSEG plugin 
using the same SDK installation. You only need to create the plugin project one 
time. Once a plugin project is created, it can be accessed and built on all 
platforms supported by Pointwise.

In the sections below, the `PluginSDK` folder is shorthand for the full path 
to the `PluginSDK` folder of your SDK installation. That is, `PluginSDK` 
is shorthand for `\some\long\path\to\your\install\of\PluginSDK`.

### Creating the UMCPSEG Plugin Project on Unix and Mac OS/X
   * `% cd PluginSDK`
   * `% mkplugin -uns -cpp MyUMCPSEG`
   * `% make CaeUnsMyUMCPSEG-dr`

### Building the UMCPSEG Plugin Project on Windows

#### At the Command Prompt
 * `C:> cd PluginSDK`
 * `C:> mkplugin -uns -cpp MyUMCPSEG`
 * `C:> exit`

#### In Visual Studio 2015

 * Choose the *File &gt; Open &gt; Project/Solution...* menu
 * Select the file `PluginSDK\PluginSDK_vs2015.sln`
 * In the *Solution Explorer* window
  * Right-click on the *Plugins* folder
  * Choose *Add &gt; Existing Project...* in the popup menu
  * Select `PluginSDK\src\plugins\CaeUnsMyUMCPSEG\CaeUnsMyUMCPSEG.vcxproj`
  * Right-click on the *CaeUnsMyUMCPSEG* project
  * Choose *Build* in the popup menu

If all goes well above, you should be able to build the CaeUnsMyUMCPSEG plugin 
without errors or warnings. The plugin does not do anything yet. Building at this 
point was done to be sure we are ready to integrate the UMCPSEG source code 
from this repository.

### Integrating the github UMCPSEG Plugin Source Code into Your SDK

Once the `CaeUnsMyUMCPSEG` project is created and builds correctly as 
explained above, integrate the source code from this repository into 
your project by copying the repository files into the 
`PluginSDK\src\plugins\CaeUnsMyUMCPSEG` folder. This overwrites the files 
generated by `mkplugin`.

Edit the file `PluginSDK\src\plugins\CaeUnsMyUMCPSEG\rtCaepInitItems.h`. 
Change the plugin's name from

> ```
> "UMCPSEG",     /* const char *name */
> ```

to

> ```
> "MyUMCPSEG",   /* const char *name */
> ```

Save the file.

You can now build the `CaeUnsMyUMCPSEG` plugin on your platform. If all 
builds correctly, you now have a CaeUnsMyUMCPSEG plugin that is functionally 
equivalent to the UMCPSEG plugin distributed with Pointwise. You are now ready 
to make your desired changes to the CaeUnsMyUMCPSEG plugin.

### Closing Thoughts

In the [How to Create a CAE Plugin][SDKbuild] section, click on the 
**4. Build the CAE plugin** link for information on configuring your SDK build 
environment.

The plugin created above will appear in the Pointwise user interface and in the 
Glyph scripting language using the name 'MyUMCPSEG'. Even though it is 
functionally equivalent, Pointwise treats it as a different solver.

If you plan on publicly releasing source or binary builds of your version of 
the UMCPSEG plugin, you need to be careful about your choice of site id, site 
group name, plugin id, and plugin name. See documentation for [site.h][SDKsite.H] 
for details. Your plugin may not load correctly into Pointwise if the values you 
choose conflict with other CAE plugins.

Being open source, we would like to see any enhancements or bug fixes you make 
to the UMCPSEG plugin integrated back into the main Pointwise build. Please do 
not hesitate to send us git pull requests for your changes. We will evaluate 
your changes and integrate them as appropriate.


[SDKdownload]: http://www.pointwise.com/plugins/#sdk_downloads
[SDKdocs]: http://www.pointwise.com/plugins
[SDKsite.H]: http://www.pointwise.com/plugins/html/d6/d89/site_8h.html
[SDKbuild]: http://www.pointwise.com/plugins/html/index.html#how_to_create_a_cae_plugin
