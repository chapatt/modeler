import SwiftUI

@main
struct ModelerApp: App {
    var body: some Scene {
        WindowGroup {
            GeometryReader { geometry in
                ModelerViewControllerRepresentable(titlebarHeight: Float(geometry.safeAreaInsets.top))
                    .edgesIgnoringSafeArea(.all)
            }
        }.windowStyle(.hiddenTitleBar)
    }
}
