import AppKit

class ModelerView: NSView, CALayerDelegate {
    private var trackingArea: NSTrackingArea!
    private var inputQueue: UnsafeMutablePointer<Queue>

    override init(frame frameRect: NSRect) {
        self.inputQueue = createQueue()
        super.init(frame: frameRect)
        self.wantsLayer = true
        let layer = CAMetalLayer()
        self.layer = layer
        layer.delegate = self
        layer.setNeedsDisplay()
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    func display(_ layer: CALayer) {
        let trackingOptions: NSTrackingArea.Options = [.activeAlways, .inVisibleRect, .mouseEnteredAndExited, .mouseMoved]
        trackingArea = NSTrackingArea(rect: bounds, options: trackingOptions, owner: self, userInfo: nil)
        self.addTrackingArea(trackingArea)
        
        let layerPointer: UnsafeMutableRawPointer = Unmanaged.passUnretained(layer).toOpaque()
        let errorPointerPointer: UnsafeMutablePointer<UnsafeMutablePointer<CChar>?>? = UnsafeMutablePointer.allocate(capacity: 1)
            
        let frame: CGRect = layer.frame
        let width = Int32(frame.size.width)
        let height = Int32(frame.size.height)
        
        let resourcePath = Bundle.main.resourcePath!
        
        resourcePath.withCString { resourcePathCString in
            if (!initVulkanMetal(layerPointer, width, height, resourcePathCString, inputQueue, errorPointerPointer)) {
                if let pointerPointer = errorPointerPointer, let pointer = pointerPointer.pointee {
                    if let error: String = String(validatingUTF8: pointer) {
                        self.handleFatalError(message: error)
                    }
                }
            }
        }
    }
    
    override func mouseDown(with event: NSEvent) {
        print("mouseDown")
        if event.type == .leftMouseDown {
            print("main: mouseDown")
        }
        enqueueInputEvent(inputQueue, MOUSE_DOWN)
    }
    
    override func mouseMoved(with event: NSEvent) {
        print("mouseMoved")
        if event.type == .mouseMoved {
            print("mouseMoved")
        }
    }
    
    override func mouseDragged(with event: NSEvent) {
        print("mouseDragged")
        if event.type == .leftMouseDragged {
            print("mouseDragged")
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
