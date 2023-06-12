import SwiftUI

struct ModelerViewRepresentable: NSViewRepresentable {
    typealias NSViewType = ModelerView
    
    func makeNSView(context: Context) -> Self.NSViewType {
        return Self.NSViewType();
    }
    
    func updateNSView(_ nsView: Self.NSViewType, context: Context) {
    }
}
