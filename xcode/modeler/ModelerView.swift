import AppKit

class ModelerView: NSView, CALayerDelegate {
    override init(frame frameRect: NSRect) {
        super.init(frame: frameRect)
        self.wantsLayer = true
        self.layer = CAMetalLayer()
        if let layer = self.layer {
            layer.delegate = self
            layer.setNeedsDisplay()
        }
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    func display(_ layer: CALayer) {
        layer.backgroundColor = NSColor.blue.cgColor
        
        let layerPointer: UnsafeMutableRawPointer = Unmanaged.passUnretained(layer).toOpaque()
        let errorPointerPointer: UnsafeMutablePointer<UnsafeMutablePointer<CChar>?>? = UnsafeMutablePointer.allocate(capacity: 1)
            
        let frame: CGRect = layer.frame
        let width = Int32(frame.size.width)
        let height = Int32(frame.size.height)
        
        let resourcePath = Bundle.main.resourcePath!
        
        DispatchQueue.global(qos: .userInitiated).async {
            resourcePath.withCString { resourcePathCString in
                if (!initVulkanMetal(layerPointer, width, height, resourcePathCString, errorPointerPointer)) {
                    if let pointerPointer = errorPointerPointer, let pointer = pointerPointer.pointee {
                        if let error: String = String(validatingUTF8: pointer) {
                            self.handleFatalError(message: error)
                        }
                    }
                }
            }
        }
    }
    
    func handleFatalError(message: String) {
        let alert = NSAlert()
        alert.messageText = "Modeler Error"
        alert.informativeText = message
        alert.addButton(withTitle: "OK")
        alert.alertStyle = .critical
        alert.runModal()
        NSApplication.shared.terminate(nil)
    }
}
