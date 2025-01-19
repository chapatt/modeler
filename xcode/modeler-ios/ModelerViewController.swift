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
        
        NotificationCenter.default.addObserver(self, selector: #selector(self.handleWillEnterForeground), name: UIApplication.willEnterForegroundNotification, object: nil)
        
        NotificationCenter.default.addObserver(self, selector: #selector(self.handleDidEnterBackground), name: UIApplication.didEnterBackgroundNotification, object: nil)
    }

    override func viewDidDisappear(_ animated: Bool) {
        (view as! ModelerView).terminateVulkanThread()
    }
    
    @objc func handleWillEnterForeground() {
        (view as! ModelerView).startVulkanThread()
    }
    
    @objc func handleDidEnterBackground() {
        (view as! ModelerView).terminateVulkanThread()
    }
}
