import AppKit

class ModelerViewController: NSViewController {
    override func loadView() {
        view = ModelerView()
    }
    
    override func viewDidAppear() {
        (view as! ModelerView).startVulkanThread()
    }

    override func viewDidDisappear() {
        (view as! ModelerView).terminateVulkanThread()
    }
}
