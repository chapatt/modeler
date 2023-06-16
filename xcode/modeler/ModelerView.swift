import AppKit

class ModelerView: NSView {
    override init(frame frameRect: NSRect) {
        super.init(frame: frameRect)
        self.wantsLayer = true
        self.layer = CAMetalLayer()
        if let layer = self.layer {
            layer.backgroundColor = NSColor.blue.cgColor
            let layerPointer: UnsafeMutableRawPointer = Unmanaged.passUnretained(layer).toOpaque()
            initVulkanMetal(layerPointer)
        }
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
}
