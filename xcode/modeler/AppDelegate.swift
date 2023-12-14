import Cocoa

class AppDelegate: NSObject, NSApplicationDelegate {
    private var window: NSWindow!
    
    let windowController = ModelerWindowController(
        window: ModelerWindow(
            contentRect: NSRect(x: 0, y: 0, width: 600, height: 400),
            styleMask: [.borderless],
            backing: .buffered,
            defer: false))

    func applicationDidFinishLaunching(_ aNotification: Notification) {
        windowController.window?.delegate = windowController
        windowController.showWindow(self)
    }
    
    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        return true
    }
    
    func applicationWillTerminate(_ notification: Notification) {
        print("terminating")
    }
}
