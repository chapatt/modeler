import AppKit

class ModelerViewController: NSViewController {
    convenience init(view: NSView) {
        self.init()
        self.view = view
    }
    
    override func loadView() {
        // Deliberately left empty
    }
}
