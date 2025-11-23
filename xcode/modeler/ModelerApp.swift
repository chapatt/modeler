import SwiftUI

@main
struct ModelerApp: App {
    var body: some Scene {
        WindowGroup {
            GeometryReader { geometry in
                ModelerViewControllerRepresentable(titlebarHeight: Float(30))
                    .ignoresSafeArea()
            }
        }.windowStyle(.hiddenTitleBar)
    }
}
