import SwiftUI

@main
struct modelerApp: App {
    @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate

    var body: some Scene {
        WindowGroup {
            ModelerViewRepresentable()
        }
    }
}
