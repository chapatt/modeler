import SwiftUI

struct ModelerViewControllerRepresentable: NSViewControllerRepresentable {
    @State var titlebarHeight: Float
    typealias NSViewControllerType = ModelerViewController
    
    func makeNSViewController(context: Context) -> ModelerViewController {
        let viewController = ModelerViewController()
        viewController.titlebarHeight = titlebarHeight + 7
        
        return viewController
    }
    
    func updateNSViewController(_ nsViewController: ModelerViewController, context: Context) {
    }
}
