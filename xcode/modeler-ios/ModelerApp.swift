import SwiftUI

@main
struct ModelerApp: App {
    var body: some Scene {
        WindowGroup {
            ModelerViewControllerRepresentable()
                .ignoresSafeArea()
        }
    }
}
