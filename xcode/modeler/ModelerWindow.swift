import Cocoa

class ModelerWindow: NSWindow {
    override var canBecomeKey: Bool {
        return true
    }
    
    override var canBecomeMain: Bool {
        return true
    }
    
    override init(contentRect: NSRect, styleMask: NSWindow.StyleMask, backing: NSWindow.BackingStoreType, defer: Bool) {
        super.init(
            contentRect: contentRect,
            styleMask: [.borderless],
            backing: .buffered,
            defer: false)

        self.title = "Modeler"
        self.isOpaque = false
        self.center()
        self.backgroundColor = NSColor.clear
        self.contentView = ModelerView()
        self.makeKeyAndOrderFront(nil)
        self.acceptsMouseMovedEvents = true
    }
}
