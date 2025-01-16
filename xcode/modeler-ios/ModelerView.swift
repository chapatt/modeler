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
        layer.setNeedsDisplay()
        
        errorPointerPointer = UnsafeMutablePointer.allocate(capacity: 1)
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
    
    override func display(_ layer: CALayer) {
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
    
    func handleFatalError(message: String) {
    }
}
