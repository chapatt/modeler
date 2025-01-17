import SwiftUI

struct ModelerViewControllerRepresentable: NSViewControllerRepresentable {
    typealias NSViewControllerType = ModelerViewController
    
    func makeNSViewController(context: Context) -> ModelerViewController {
        ModelerViewController()
    }
    
    func updateNSViewController(_ nsViewController: ModelerViewController, context: Context) {
    }
}
