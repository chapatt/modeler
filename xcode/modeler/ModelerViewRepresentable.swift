import SwiftUI

struct ModelerViewRepresentable: NSViewRepresentable {
    typealias NSViewType = ModelerView
    
    func makeNSView(context: Context) -> ModelerView {
        ModelerView()
    }
    
    func updateNSView(_ nsView: ModelerView, context: Context) {
    }
}
