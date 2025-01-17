import SwiftUI

@main
struct ModelerApp: App {
    var body: some Scene {
        WindowGroup {
            ModelerViewRepresentable()
                .ignoresSafeArea()
        }.windowStyle(.hiddenTitleBar)
    }
}
