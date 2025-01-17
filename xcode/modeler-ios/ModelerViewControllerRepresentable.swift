import SwiftUI

struct ModelerViewControllerRepresentable: UIViewControllerRepresentable {
    typealias UIViewControllerType = ModelerViewController
    
    func makeUIViewController(context: Context) -> ModelerViewController {
        ModelerViewController()
    }
    
    func updateUIViewController(_ uiViewController: ModelerViewController, context: Context) {
    }
}
