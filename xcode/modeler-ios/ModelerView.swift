import UIKit

class ModelerView: UIView {
    private var inputQueue: UnsafeMutablePointer<Queue>
    private var thread: pthread_t?
    private var errorPointerPointer: UnsafeMutablePointer<UnsafeMutablePointer<CChar>?>?
    
    override class var layerClass: AnyClass {
        return CAMetalLayer.self
    }
    
    override init(frame: CGRect) {
        self.inputQueue = createQueue()
        super.init(frame: frame)
        
        errorPointerPointer = UnsafeMutablePointer.allocate(capacity: 1)
        
        NotificationCenter.default.addObserver(self, selector: #selector(self.handleErrorNotification), name: Notification.Name("THREAD_FAILURE"), object: nil)
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func didMoveToWindow() {
        if let nativeScale = self.window?.windowScene?.screen.nativeScale {
            self.contentScaleFactor = nativeScale
        }
        layer.delegate = self
    }
    
    func startVulkanThread() {
        if let layer = (layer as? CAMetalLayer) {
            var drawableSize = bounds.size
            drawableSize.width *= contentScaleFactor
            drawableSize.height *= contentScaleFactor
            layer.drawableSize = drawableSize
            
            let layerPointer: UnsafeMutableRawPointer = Unmanaged.passUnretained(layer).toOpaque()
            
            let width = Int32(layer.drawableSize.width)
            let height = Int32(layer.drawableSize.height)
            let scale = Float(contentScaleFactor);
            
            let resourcePath = Bundle.main.resourcePath!
            
            resourcePath.withCString { resourcePathCString in
                thread = initVulkanMetal(layerPointer, width, height, scale, resourcePathCString, inputQueue, errorPointerPointer)
                
                if (thread == nil) {
                    if let pointerPointer = errorPointerPointer, let pointer = pointerPointer.pointee {
                        if let error: String = String(validatingUTF8: pointer) {
                            self.handleFatalError(message: error)
                        }
                    }
                }
            }
        }
    }
    
    func terminateVulkanThread() {
        terminateVulkan(inputQueue, thread)
    }

    @objc func handleErrorNotification(notification: NSNotification) {
        if let pointerPointer = errorPointerPointer, let pointer = pointerPointer.pointee {
            if let error: String = String(validatingUTF8: pointer) {
                handleFatalError(message: error)
            }
        }
    }
    
    func handleLayoutChanged() {
        print("orientation changed\n")
        let extent = VkExtent2D(width: UInt32(bounds.size.width), height: UInt32(bounds.size.height))
        let rect = VkRect2D(offset: VkOffset2D(x: 0, y: 0), extent: extent)
        var orientation = ROTATE_0
        switch UIDevice.current.orientation {
        case .portrait:
            orientation = ROTATE_0
        case .portraitUpsideDown:
            orientation = ROTATE_180
        case .landscapeLeft:
            orientation = ROTATE_90
        case .landscapeRight:
            orientation = ROTATE_270
        default:
            orientation = ROTATE_0
        }
        let windowDimensions = WindowDimensions(
            surfaceArea: extent,
            activeArea: rect,
            cornerRadius: 0,
            scale: Float(contentScaleFactor),
            fullscreen: false,
            orientation: orientation
        )
        let layerPointer: UnsafeMutableRawPointer = Unmanaged.passUnretained(layer).toOpaque()
            
        enqueueResizeEvent(inputQueue, windowDimensions, layerPointer)
    }

    func handleFatalError(message: String) {
    }
}
