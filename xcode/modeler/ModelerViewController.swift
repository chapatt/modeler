import AppKit

class ModelerViewController: NSViewController {
    var titlebarHeight: Float = 0
    
    override init(nibName nibNameOrNil: NSNib.Name?, bundle nibBundleOrNil: Bundle?) {
        super.init(nibName: nibNameOrNil, bundle: nibBundleOrNil)
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func loadView() {
        view = ModelerView()
        (view as! ModelerView).titlebarHeight = titlebarHeight
    }
    
    override func viewDidAppear() {
        (view as! ModelerView).startVulkanThread()
    }

    override func viewDidDisappear() {
        (view as! ModelerView).terminateVulkanThread()
    }
}
