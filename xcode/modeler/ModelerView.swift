import AppKit

class ModelerView: NSView, CALayerDelegate {
    private var trackingArea: NSTrackingArea!
    private var inputQueue: UnsafeMutablePointer<Queue>
    private var thread: pthread_t?
    private var errorPointerPointer: UnsafeMutablePointer<UnsafeMutablePointer<CChar>?>?

    override init(frame frameRect: NSRect) {
        self.inputQueue = createQueue()
        super.init(frame: frameRect)
        self.postsFrameChangedNotifications = true
        self.wantsLayer = true
        let layer = CAMetalLayer()
        self.layer = layer
        layer.delegate = self
        layer.setNeedsDisplay()
        
        errorPointerPointer = UnsafeMutablePointer.allocate(capacity: 1)
        
        NotificationCenter.default.addObserver(self, selector: #selector(self.handleErrorNotification), name: Notification.Name("THREAD_FAILURE"), object: nil)
        
        NotificationCenter.default.addObserver(self, selector: #selector(self.handleFrameDidChange), name: NSView.frameDidChangeNotification, object: nil)
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    func display(_ layer: CALayer) {
        let trackingOptions: NSTrackingArea.Options = [.activeAlways, .inVisibleRect, .mouseEnteredAndExited, .mouseMoved]
        trackingArea = NSTrackingArea(rect: bounds, options: trackingOptions, owner: self, userInfo: nil)
        self.addTrackingArea(trackingArea)
        
        let layerPointer: UnsafeMutableRawPointer = Unmanaged.passUnretained(layer).toOpaque()
            
        let bounds: CGRect = layer.bounds
        let width = Int32(bounds.size.width)
        let height = Int32(bounds.size.height)
        
        let resourcePath = Bundle.main.resourcePath!

        resourcePath.withCString { resourcePathCString in
            thread = initVulkanMetal(layerPointer, width, height, resourcePathCString, inputQueue, errorPointerPointer)
            
            if (thread == nil) {
                if let pointerPointer = errorPointerPointer, let pointer = pointerPointer.pointee {
                    if let error: String = String(validatingUTF8: pointer) {
                        self.handleFatalError(message: error)
                    }
                }
            }
        }
    }
    
    func terminateVulkanThread() {
        terminateVulkan(inputQueue, thread)
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

    @objc func handleErrorNotification(notification: NSNotification) {
        if let pointerPointer = errorPointerPointer, let pointer = pointerPointer.pointee {
            if let error: String = String(validatingUTF8: pointer) {
                handleFatalError(message: error)
            }
        }
    }
    
    @objc func handleFrameDidChange(object: NSView) {
        print("extentChange")
        enqueueInputEvent(inputQueue, EXTENT_CHANGE)
    }
    
    func handleFatalError(message: String) {
        DispatchQueue.main.async {
            let alert = NSAlert()
            alert.messageText = "Modeler Error"
            alert.informativeText = message
            alert.addButton(withTitle: "OK")
            alert.alertStyle = .critical
            alert.runModal()
            NSApplication.shared.terminate(nil)
        }
    }
}
