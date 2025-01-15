import UIKit

class ModelerViewController: UIViewController {
    override func loadView() {
        self.view = ModelerView(frame: CGRect(x: 0.0, y: 0.0, width: 100.0, height: 100.0))
    }

    override func viewDidLoad() {
        super.viewDidLoad()
    }
    
    override func viewDidAppear(_ animated: Bool) {
        if let view = self.view as? ModelerView {
            view.startRenderloop()
        }
    }
}
