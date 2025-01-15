import Cocoa

class ModelerWindowController: NSWindowController, NSWindowDelegate {
    override func windowDidLoad() {
        super.windowDidLoad()
    }

    func windowWillClose(_ notification: Notification) {
        (window?.contentView as! ModelerView).terminateVulkanThread()
    }
}
