import Cocoa

class ModelerWindowController: NSWindowController, NSWindowDelegate {
    override func windowDidLoad() {
        super.windowDidLoad()
    }

    func windowWillClose(_ notification: Notification) {
        print("closing")
        (window?.contentView as! ModelerView).terminateVulkan()
    }
}
