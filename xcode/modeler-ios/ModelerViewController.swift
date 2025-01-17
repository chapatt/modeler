import UIKit

class ModelerViewController: UIViewController {
    override func loadView() {
        view = ModelerView()
    }
    
    override func viewWillLayoutSubviews() {
        (view as! ModelerView).handleLayoutChanged()
    }
    
    override func viewDidAppear(_ animated: Bool) {
        (view as! ModelerView).startVulkanThread()
    }

    override func viewDidDisappear(_ animated: Bool) {
        (view as! ModelerView).terminateVulkanThread()
    }
}
