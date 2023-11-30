import Cocoa

class AppDelegate: NSObject, NSApplicationDelegate {
    private var window: NSWindow!

    func applicationDidFinishLaunching(_ aNotification: Notification) {
        createWindow()
    }
    
    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        return true
    }
    
    func createWindow() {
        let rect = NSRect(
            x: 0,
            y: 0,
            width: NSScreen.main!.frame.midX,
            height: NSScreen.main!.frame.midY)
    
        window = ModelerWindow(
            contentRect: rect,
            styleMask: [.borderless],
            backing: .buffered,
            defer: false)
    }
}
