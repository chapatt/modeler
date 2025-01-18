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
    
    func exitApp(_: UIAlertAction) {
        exit(EXIT_FAILURE)
    }

    func handleFatalError(message: String) {
        DispatchQueue.main.async {
            let alert = UIAlertController(title: "Modeler Error", message: message, preferredStyle: UIAlertController.Style.alert)
            alert.addAction(UIAlertAction(title: "OK", style: UIAlertAction.Style.default, handler: self.exitApp))
            let windowScenes = UIApplication.shared.connectedScenes.compactMap { $0 as? UIWindowScene }
            let activeScenes = windowScenes.filter { $0.activationState == .foregroundActive }
            let keyWindow = activeScenes.first?.keyWindow
            let viewController = keyWindow?.rootViewController
            viewController?.present(alert, animated: true, completion: nil)
        }
    }
    
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        if let touch = touches.first {
            enqueuePositionEventWithWindowCoord(touch.location(in: self))
            enqueueInputEvent(inputQueue, BUTTON_DOWN, nil)
        }
    }
     
    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        if let touch = touches.first {
            touch.location(in: self)
            enqueuePositionEventWithWindowCoord(touch.location(in: self))
        }
    }
     
    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        if let touch = touches.first {
            touch.location(in: self)
            enqueuePositionEventWithWindowCoord(touch.location(in: self))
            enqueueInputEvent(inputQueue, BUTTON_UP, nil)
        }
    }
     
    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        if let touch = touches.first {
            touch.location(in: self)
            enqueuePositionEventWithWindowCoord(touch.location(in: self))
            enqueueInputEvent(inputQueue, BUTTON_UP, nil)
        }
    }
    
    private func enqueuePositionEventWithWindowCoord(_ windowCoord: CGPoint) {
        let x = Int32(windowCoord.x)
        let y = Int32(windowCoord.y)
        enqueueInputEventWithPosition(inputQueue, POINTER_MOVE, x, y)
    }
}
