# CoreCLR Single-file Publish
Design for publishing apps as a single-file in CoreCLR 3.0

### The options
There are several options to implement Single-file apps, with varying cost and impact. For example:

1. Self-Extractor: Extract dependencies from the bundle to temporary files.
2. Super BootStrapper: Execute directly from bundle.
3. CoreRT: Native compile and link managed app and its dependencies.

#### Developer feedback

Several developers provided feedback regarding this feature in this issue [#20287](https://github.com/dotnet/coreclr/issues/20287)

* There is considerable enthusiasm and interest in using single-file deployment.
* But not with the self-extractor behavior.

Bundling to single-file and extracting to temporary files is not new technology, but integrating it inbox into dotnet cli is a non-trivial amount of work. So, we need more data to make sure that it is interesting to some customers before investing in it.

##### Open questions:

There are a few different questions to be answered, for what people want:

* Whether developers want to use it purely for packaging? or is it because of performance expectations? 
For packaging:
	* Does the solution need to be inbox (dotnet-cli), or are devs happy to use (existing/new) third party tools?
	* For self-extractor, do customers like:
		* An install-like experience? (extract files to a standard directory, standard cleanup procedure, etc?)
		* App-run like experience? (extract to local directory or temp directory)
* If going for super-bootstraper like experience 
	* Are customers willing to use a solution that may not support only limited situations? (in the first release)? 
	For example:
	* Only support embedding and direct-loading MSIL images.
	* Limited debugging support from the single-file? 

#### Self-Extractor

For CoreCLR 3.0 release, the management has asked for the Self-extractor solution.

Pros:

* Most compatible: most apps run without any change
* Least cost: this is the only implementation that fits the schedule.
 
Cons:

* Doesn’t work in non-writable environments (which is likely a small set relatively)
* Not very different from what people can already do independently using other tools

The rest of this document explains the design to extract 

## Design

There are two parts to publishing apps as a single file:

* A bundler tool to put dependencies and the app together in a single file.
* The support by the runtime/host to extract the bundled resources.

### The Bundler

#### Windows

#### Unix 

### The Extractor

#### Where to Extract? 
 We need to account for several scenarios:

* First launch: the app just needs to extract to somewhere on disk
* Subsequent launches -- to avoid paying the cost of extraction (likely several seconds) on every launch, it would be preferable to have the extraction location be deterministic and allow the second launch to use files extracted by the first launch.
* Upgrade: If a new version of the application is launched, it shouldn't use the files extracted by an old version. (The reverse is also true; people may want to run multiple version side-by-side). That suggests that the deterministic path should be based on the contents of the application.
* Uninstall: Users should be able to find the extracted directories to delete them if desired.
* Fault-tolerance: If a first launch fails after partially extracting its contents, a second launch should redo the extraction
* Running elevated: Processes run as admin should only run from admin-writable locations to prevent low-integrity processes from tampering with them.
* Running non-elevated: Processes run without admin privileges should run from user-writable locations

We can account for all of those by constructing a path that incorporates:

* A well-known base directory (e.g. `%LOCALAPPDATA%\dotnetApps` on Windows and user user profile locations on other OSes)
* A separate subdirectory for elevated
* Application identity (maybe just the exe name)
* A version identifier. The number version is probably useful, but insufficient since it also needs to incorporate exact dependency versions. A per-build guid or hash might be appropriate.

Together, that might look something like `c:\users\username\AppData\Local\dotnetApps\elevated\MyCoolApp\1.0.0.0_abc123\MyCoolApp.dll`
(Where the app is named `MyCoolApp`, its version number is `1.0.0.0` and its hash/guid is `abc123` and it was launched elevated).

There will also be work required to embed files into the extractor. On Windows, we can simply use native resources, but Linux and Mac may need custom work.

#### Host Support

#### Dependency Resolution



