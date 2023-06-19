import AppKit

class ModelerView: NSView {
    override init(frame frameRect: NSRect) {
        super.init(frame: frameRect)
        self.wantsLayer = true
        self.layer = CAMetalLayer()
        if let layer = self.layer {
            layer.backgroundColor = NSColor.blue.cgColor
            let layerPointer: UnsafeMutableRawPointer = Unmanaged.passUnretained(layer).toOpaque()

            let errorPointerPointer: UnsafeMutablePointer<UnsafeMutablePointer<CChar>?>? = UnsafeMutablePointer.allocate(capacity: 1)
            if (!initVulkanMetal(layerPointer, errorPointerPointer)) {
                if let pointerPointer = errorPointerPointer, let pointer = pointerPointer.pointee {
                    if let error: String = String(validatingUTF8: pointer) {
                        handleFatalError(message: error)
                    }
                }
            }
        }
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    func handleFatalError(message: String) {
        let alert = NSAlert()
        alert.messageText = "Modeler Error"
        alert.informativeText = message
        alert.addButton(withTitle: "OK")
        alert.alertStyle = .critical
        alert.runModal()
        exit(EXIT_FAILURE)
    }
}
