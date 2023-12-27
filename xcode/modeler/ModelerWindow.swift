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
            styleMask: [
                .titled,
                .fullSizeContentView,
                .closable,
                .miniaturizable,
                .resizable
            ],
            backing: .buffered,
            defer: false)
        
        self.contentView = ModelerView()

        self.title = "Modeler"
        self.isOpaque = false
        self.center()
        self.backgroundColor = NSColor.clear
        self.makeKeyAndOrderFront(nil)
        self.acceptsMouseMovedEvents = true
        self.titleVisibility = .hidden
        self.titlebarAppearsTransparent = true
    }
}
