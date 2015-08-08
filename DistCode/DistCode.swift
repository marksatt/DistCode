//
//  DistCode.swift
//  DistCode
//
//  Created by Mark Satterthwaite on 20/01/2015.
//  Copyright (c) 2015 marksatt. All rights reserved.
//

import Cocoa

struct DistTask
{
	var Pipe: NSPipe!
	var Task: NSTask!
}

func BeginTask(Path: NSString, Arguments: NSArray, Pipe: NSPipe) -> NSTask
{
	let task = NSTask()
	task.arguments = Arguments as [AnyObject]
	task.launchPath = Path as String
	task.standardOutput = Pipe
	task.launch()
	return task
}

func StopTask(Task: NSTask, Force: BooleanType)
{
	if(Force)
	{
		Task.terminate()
	}
	Task.waitUntilExit();
}

func ExecuteTask(Path: NSString, Arguments: NSArray) -> NSString!
{
	let pipe = NSPipe()
	let file = pipe.fileHandleForReading
	let task = BeginTask(Path, Arguments, pipe)
	StopTask(task, false)
	let data = file.readDataToEndOfFile()
	let string = NSString(data:data, encoding:NSUTF8StringEncoding)
	return string
}

func CompareVersion(Version:NSString, toVersion:NSString) -> NSComparisonResult
{
	let versionOneComp: NSArray = Version.componentsSeparatedByString(".");
	let versionTwoComp: NSArray = toVersion.componentsSeparatedByString(".");
	
	var pos = 0;
	
	while (versionOneComp.count > pos || versionTwoComp.count > pos)
	{
		let v1 = versionOneComp.count > pos ? versionOneComp.objectAtIndex(pos).integerValue : 0;
		let v2 = versionTwoComp.count > pos ? versionTwoComp.objectAtIndex(pos).integerValue : 0;
		if (v1 < v2) {
			return NSComparisonResult.OrderedAscending;
		}
		else if (v1 > v2) {
			return NSComparisonResult.OrderedDescending;
		}
		pos++;
	}
	
	return NSComparisonResult.OrderedSame;
}

let kDistCodeGroupIdentifier = "DistCode"
let kHostsItemIdentifier = "Hosts"
let kMonitorItemIdentifier = "Monitor"
let kOptionsItemIdentifier = "Options"

class DistCodeAppDelegate : NSObject, NSApplicationDelegate, DOTabbarDelegate {
	
	@IBOutlet weak var Window: OBMenuBarWindow!
	@IBOutlet weak var Tabbar: DOTabbar!
	@IBOutlet weak var TabView: NSTabView!
	@IBOutlet weak var DistCCServerController: NSArrayController!
	@IBOutlet weak var TasksController: NSArrayController!
	var IceccScheduler: DistTask
	var IceccDaemon: DistTask
	var MonitorTimer: NSTimer!
	var IceCodeWrapper: IceCode!
	
	override init()
	{
		IceccScheduler = DistTask(Pipe: nil, Task: nil)
		IceccDaemon = DistTask(Pipe: nil, Task: nil)
		super.init()
	}
	
	deinit
	{
		
	}
	
	func monitorEvent()
	{
		if(self.IceCodeWrapper != nil)
		{
			self.DistCCServerController.content = self.IceCodeWrapper.hostsArray()
			self.TasksController.content = self.IceCodeWrapper.jobsArray()
		}
	}
	
	func applicationDidFinishLaunching(notification: NSNotification)
	{
		// Insert code here to initialize your application
		let Image:NSImage = NSImage(named:"vpn-512")!
		Image.size = NSMakeSize(22, 22)
		self.Window.menuBarIcon = Image;
		self.Window.highlightedMenuBarIcon = Image;
		self.Window.hasMenuBarIcon = true;
		self.Window.attachedToMenuBar = true;
		self.Window.isDetachable = true;
		
		let DefaultsPath = NSBundle.mainBundle().pathForResource("Defaults", ofType:"plist") as NSString?
		if (DefaultsPath != nil)
		{
			let Dict = NSDictionary(contentsOfFile:DefaultsPath! as String)
			NSUserDefaults.standardUserDefaults().registerDefaults(Dict! as [NSObject : AnyObject])
		}
		
		self.CheckAndInstall()
		self.UpdateXcodePath()
		
		let bCoordinatorMode = NSUserDefaults.standardUserDefaults().valueForKey("CoordinatorMode")?.boolValue
		if(bCoordinatorMode == false)
		{
			self.StartIceccScheduler()
		}
		
		self.StartIceccDaemon()
		self.Tabbar.selectItemWithIdentifier(kHostsItemIdentifier);
		
		let TimeoutObj = NSUserDefaults.standardUserDefaults().valueForKey("HostTimeout") as! NSNumber?
		let Timeout: NSInteger = (TimeoutObj != nil && TimeoutObj!.integerValue > 0) ? TimeoutObj!.integerValue : 10
		let Port:UInt = 0
		let NetName = ""
		var Scheduler = ""
		if(bCoordinatorMode == true)
		{
			let schedulerIP = NSUserDefaults.standardUserDefaults().valueForKey("CoordinatorIP") as! NSString?
			if(schedulerIP != nil)
			{
				Scheduler = schedulerIP! as String
			}
		}
		self.IceCodeWrapper = IceCode(forNetName: NetName, withTimeout: Timeout, andSchedulerName: Scheduler, andPort: Port)
		
		MonitorTimer = NSTimer.scheduledTimerWithTimeInterval(2, target: self, selector: Selector("monitorEvent"), userInfo: nil, repeats: true)
	}
	
	func applicationWillTerminate(notification: NSNotification)
	{
		self.StopIceccDaemon()
		self.StopIceccScheduler()
		self.IceCodeWrapper = nil
		// Set the number of Xcode build tasks.
		let XcodeDefaults:NSUserDefaults = NSUserDefaults(suiteName:"com.apple.dt.Xcode")!
		// Don't ever use 0, if we get to that use the system default.
		XcodeDefaults.removeObjectForKey("IDEBuildOperationMaxNumberOfConcurrentCompileTasks");
		XcodeDefaults.synchronize()
	}
	
	// MARK: - DOTabBar
	func setViewForIdentifier(identifier:NSString)
	{
		self.TabView.selectTabViewItemWithIdentifier(identifier);
	}
	
	func tabbar(tabbar: DOTabbar!, didSelectItemWithIdentifier identifier: String!)
	{
		self.setViewForIdentifier(identifier)
	}
	
	func tabbarGroupIdentifiers(tabbar: DOTabbar!) -> [AnyObject]! {
		return [kDistCodeGroupIdentifier]
	}
	
	func tabbar(tabbar: DOTabbar!, titleForGroupIdentifier identifier: String!) -> String! {
		return nil;
	}
	
	func tabbar(tabbar: DOTabbar!, itemIdentifiersForGroupIdentifier identifier: String!) -> [AnyObject]! {
		if (identifier == kDistCodeGroupIdentifier)
		{
			return [kHostsItemIdentifier, kMonitorItemIdentifier, kOptionsItemIdentifier]
		}
		else
		{
			return nil;
		}
	}
	
	func tabbar(tabbar: DOTabbar!, cellForItemIdentifier itemIdentifier: String!) -> NSCell! {
		var cell = DOTabbarItemCell()
		
		if (itemIdentifier == kHostsItemIdentifier) {
			cell.title = kHostsItemIdentifier;
			cell.image = NSImage(named:"computer");
			cell.image!.size = NSMakeSize(40, 40)
		} else if (itemIdentifier == kMonitorItemIdentifier) {
			cell.title = kMonitorItemIdentifier;
			cell.image = NSImage(named:"utilities-system-monitor");
			cell.image!.size = NSMakeSize(40, 40)
		} else if (itemIdentifier == kOptionsItemIdentifier) {
			cell.title = kOptionsItemIdentifier;
			cell.image = NSImage(named:"applications-system");
			cell.image!.size = NSMakeSize(40, 40)
		}
		
		return cell;
	}
	
	func CheckAndInstall()
	{
		let Paths: NSArray = NSSearchPathForDirectoriesInDomains(NSSearchPathDirectory.ApplicationSupportDirectory, NSSearchPathDomainMask.UserDomainMask, true);
		let Path = NSString(format:"%@/Developer/Shared/Xcode/Plug-ins", Paths.objectAtIndex(0) as! NSString);
		let PluginLink = NSString(format:"%@/Icecc.xcplugin", Path);
		if(NSFileManager.defaultManager().fileExistsAtPath(PluginLink as String) == true)
		{
			let CurrentBundle = NSBundle(path: PluginLink as String);
			let CurrentDict: NSDictionary! = CurrentBundle?.infoDictionary as NSDictionary?
			let CurrentVersion: NSString = CurrentDict.objectForKey("CFBundleVersion") as! NSString;
			let PluginPath = NSBundle.mainBundle().pathForResource("Icecc", ofType: "xcplugin")
			let NewBundle = NSBundle(path: PluginPath!);
			let NewDict: NSDictionary! = NewBundle?.infoDictionary as NSDictionary?
			let NewVersion: NSString = NewDict.objectForKey("CFBundleVersion") as! NSString;
			if(CompareVersion(CurrentVersion, NewVersion) == NSComparisonResult.OrderedAscending)
			{
				NSFileManager.defaultManager().removeItemAtPath(PluginLink as String, error:nil);
			}
		}
		if(NSFileManager.defaultManager().fileExistsAtPath(PluginLink as String) == false)
		{
			NSFileManager.defaultManager().createDirectoryAtPath(Path as String, withIntermediateDirectories:true, attributes: nil, error: nil);
			let PluginPath = NSBundle.mainBundle().pathForResource("Icecc", ofType: "xcplugin")
			NSFileManager.defaultManager().copyItemAtPath(PluginPath!, toPath: PluginLink as String, error:nil);
			
			let DaemonPath = NSString(format:"%@/Contents/usr/sbin/iceccd", PluginLink)
			let LzoPath = NSString(format:"%@/Contents/usr/lib/liblzo2.2.dylib", PluginLink)
			let KillIcePath = NSString(format:"%@/Contents/usr/sbin/killice", PluginLink)
			
			let AuthPath: NSString = NSBundle.mainBundle().pathForAuxiliaryExecutable("DistServerDaemon")!
			ExecuteTask(AuthPath, ["EXEC", DaemonPath, LzoPath, KillIcePath]);
		}
		if(NSFileManager.defaultManager().fileExistsAtPath(PluginLink as String) == true)
		{
			let InfoPath = NSString(format:"%@/Contents/Info.plist", PluginLink)
			let InfoData = NSData(contentsOfFile: InfoPath as String)!
			var InfoFormat: NSPropertyListFormat = NSPropertyListFormat.XMLFormat_v1_0
			let InfoOptions: Int = Int(NSPropertyListMutabilityOptions.MutableContainersAndLeaves.rawValue)
			var InfoError: NSErrorPointer = NSErrorPointer()
			var PropertyList: AnyObject? = NSPropertyListSerialization.propertyListWithData(InfoData, options: InfoOptions, format: &InfoFormat, error: InfoError)
			if ( PropertyList != nil )
			{
				let Dictionary: NSMutableDictionary = PropertyList! as! NSMutableDictionary
				let UUIDs: NSMutableArray = Dictionary.valueForKey("DVTPlugInCompatibilityUUIDs") as! NSMutableArray
				let XcodePathResult = ExecuteTask("/usr/bin/xcode-select", ["-p"])
				let XcodePath = XcodePathResult.stringByTrimmingCharactersInSet(NSCharacterSet(charactersInString: "\n \r"))
				let XcodeInfoPath = XcodePath.stringByDeletingLastPathComponent.stringByAppendingPathComponent("Info.plist")
				let XcodeData = NSData(contentsOfFile: XcodeInfoPath)!
				
				var XcodeFormat: NSPropertyListFormat = NSPropertyListFormat.XMLFormat_v1_0
				let XcodeOptions: Int = Int(NSPropertyListMutabilityOptions.Immutable.rawValue)
				var XcodeError: NSErrorPointer = NSErrorPointer()
				var XcodePropertyList: AnyObject? = NSPropertyListSerialization.propertyListWithData(XcodeData, options: XcodeOptions, format: &XcodeFormat, error: XcodeError)
				if ( XcodePropertyList != nil )
				{
					let XcodeDictionary: NSMutableDictionary = XcodePropertyList! as! NSMutableDictionary
					let UUID: NSString = XcodeDictionary.valueForKey("DVTPlugInCompatibilityUUID") as! NSString
					if ( UUIDs.containsObject(UUID) == false )
					{
						UUIDs.addObject(UUID);
						Dictionary.setValue(UUIDs, forKey: "DVTPlugInCompatibilityUUIDs")
						let NewDict = NSPropertyListSerialization.dataFromPropertyList(Dictionary, format: InfoFormat, errorDescription: nil)
						if ( NewDict != nil )
						{
							NewDict?.writeToFile(InfoPath as String, atomically: true)
						}
					}
				}
			}
		}
	}
	
	func UpdateXcodePath()
	{
		let Result = ExecuteTask("/usr/bin/xcode-select", ["-p"])
		
		let Path = Result.stringByTrimmingCharactersInSet(NSCharacterSet(charactersInString: "\n \r"))
		
		if(NSFileManager.defaultManager().fileExistsAtPath(Path))
		{
			let PreviousPath = NSUserDefaults.standardUserDefaults().valueForKey("XcodeToolchainPath") as! NSString?
			let ToolchainPath = Path + "/Toolchains/XcodeDefault.xctoolchain/usr/bin"
			
			if(PreviousPath == nil || ToolchainPath != PreviousPath!)
			{
				NSUserDefaults.standardUserDefaults().setValue(ToolchainPath, forKey: "XcodeToolchainPath")
				let Env = getenv("PATH")
				var EnvironmentPath: String = String(UTF8String: Env)!
				if(PreviousPath != nil)
				{
					let Range = EnvironmentPath.rangeOfString(":" + (PreviousPath! as String), options: NSStringCompareOptions.CaseInsensitiveSearch, range: nil, locale: nil)
					if(Range != nil && Range?.isEmpty == false)
					{
						EnvironmentPath.removeRange(Range!)
					}
				}
				EnvironmentPath += ":" + ToolchainPath
				setenv("PATH", EnvironmentPath, 1)
			}
		}
		else
		{
			NSUserDefaults.standardUserDefaults().removeObjectForKey("XcodeToolchainPath")
		}
	}
	
	func StartIceccScheduler()
	{
		var arguments = NSMutableArray()
		let PluginPath: NSString = NSBundle.mainBundle().pathForResource("Icecc", ofType: "xcplugin")!
		let IceccSchedulerPath = NSString(format:"%@/Contents/usr/sbin/icecc-scheduler", PluginPath)
		let debugLevel = NSUserDefaults.standardUserDefaults().valueForKey("IceccDebug") as! NSNumber?
		if(debugLevel != nil && debugLevel?.integerValue > 0)
		{
			for(var i = 0; i < debugLevel?.integerValue; i++)
			{
				arguments.addObject("-v")
			}
		}
		arguments.addObject("-l")
		let logFile = NSHomeDirectory().stringByAppendingPathComponent("Library/Logs/icecc-scheduler/icecc-scheduler.log")
		NSFileManager.defaultManager().createDirectoryAtPath(logFile.stringByDeletingLastPathComponent, withIntermediateDirectories: true, attributes: nil, error: nil);
		arguments.addObject(logFile)
		IceccScheduler.Pipe = NSPipe()
		IceccScheduler.Task = BeginTask(IceccSchedulerPath, arguments, IceccScheduler.Pipe)
	}
	
	func StartIceccDaemon()
	{
		let Paths: NSArray = NSSearchPathForDirectoriesInDomains(NSSearchPathDirectory.ApplicationSupportDirectory, NSSearchPathDomainMask.UserDomainMask, true);
		let Path = NSString(format:"%@/Developer/Shared/Xcode/Plug-ins", Paths.objectAtIndex(0) as! NSString);
		let PluginLink = NSString(format:"%@/Icecc.xcplugin", Path);
		let IceccDaemonPath = NSString(format:"%@/Contents/usr/sbin/iceccd", PluginLink)
		IceccDaemon.Pipe = NSPipe()
		
		var arguments = NSMutableArray()
		let bRunCompileHost = NSUserDefaults.standardUserDefaults().valueForKey("RunCompilationHost")?.boolValue
		if(bRunCompileHost != true)
		{
			arguments.addObject("--no-remote")
		}
		let bCoordinatorMode = NSUserDefaults.standardUserDefaults().valueForKey("CoordinatorMode")?.boolValue
		if(bCoordinatorMode == true)
		{
			let schedulerIP = NSUserDefaults.standardUserDefaults().valueForKey("CoordinatorIP") as! NSString?
			if(schedulerIP != nil)
			{
				arguments.addObject("-s")
				arguments.addObject(schedulerIP!)
			}
		}
		let numJobs = NSUserDefaults.standardUserDefaults().valueForKey("MaxJobs") as! NSNumber?
		if(numJobs != nil && numJobs?.integerValue >= 0)
		{
			arguments.addObject("-m")
			let Desc = numJobs?.description
			arguments.addObject(Desc!)
		}
		let debugLevel = NSUserDefaults.standardUserDefaults().valueForKey("IceccDebug") as! NSNumber?
		if(debugLevel != nil && debugLevel?.integerValue > 0)
		{
			for(var i = 1; i <= debugLevel?.integerValue; i++)
			{
				arguments.addObject("-v")
			}
		}
		arguments.addObject("-l")
		let logFile = NSHomeDirectory().stringByAppendingPathComponent("Library/Logs/iceccd/iceccd.log")
		NSFileManager.defaultManager().createDirectoryAtPath(logFile.stringByDeletingLastPathComponent, withIntermediateDirectories: true, attributes: nil, error: nil);
		arguments.addObject(logFile)
		IceccDaemon.Task = BeginTask(IceccDaemonPath, arguments, IceccDaemon.Pipe)
	}
	
	func StopIceccDaemon()
	{
		if(IceccDaemon.Task != nil )
		{
			let Paths: NSArray = NSSearchPathForDirectoriesInDomains(NSSearchPathDirectory.ApplicationSupportDirectory, NSSearchPathDomainMask.UserDomainMask, true);
			let Path = NSString(format:"%@/Developer/Shared/Xcode/Plug-ins", Paths.objectAtIndex(0) as! NSString);
			let PluginLink = NSString(format:"%@/Icecc.xcplugin", Path);
			let KillIcePath = NSString(format:"%@/Contents/usr/sbin/killice", PluginLink)
			
			ExecuteTask(KillIcePath, NSArray());
			StopTask(IceccDaemon.Task, true)
		}
	}
	
	func StopIceccScheduler()
	{
		if(IceccScheduler.Task != nil)
		{
			StopTask(IceccScheduler.Task, true)
		}
	}
}


