import * as path from 'path';
import * as fs from 'fs';
import { workspace, ExtensionContext } from 'vscode';
import * as vscode from 'vscode';

import {
	LanguageClient,
	LanguageClientOptions,
	ServerOptions,
	TransportKind
} from 'vscode-languageclient/node';


function getGridWebviewContent(context: vscode.ExtensionContext) {
	const htmlPath = vscode.Uri.joinPath(context.extensionUri, 'client', 'media', 'grid.html');
	return fs.readFileSync(htmlPath.fsPath, 'utf8');
}

let client: LanguageClient;

export function activate(context: ExtensionContext) {
	// The server is implemented in node
	const serverPath = context.asAbsolutePath(
		path.join('..', 'bin', 'antide')
	);

	// If the extension is launched in debug mode then the debug server options are used
	// Otherwise the run options are used
	const serverOptions: ServerOptions = { 
		command: serverPath, 
		args: ["lsp", "--stdio"],
		transport: TransportKind.stdio,
	};

	// Options to control the language client
	const clientOptions: LanguageClientOptions = {
		// Register the server for plain text documents
		documentSelector: [{ scheme: 'file', pattern: "**/*.aasm" }],
		synchronize: {
			// Notify the server about file changes to '.clientrc files contained in the workspace
			fileEvents: workspace.createFileSystemWatcher('**/.clientrc')
		}
	};

	// Create the language client and start the client.
	client = new LanguageClient(
		'antideLSP',
		'Language Server for ant assembly files',
		serverOptions,
		clientOptions
	);

	// Start the client. This will also launch the server
	client.start();

	const debuggerPath = context.asAbsolutePath(
        path.join('..', 'bin', 'antide')
    );
	context.subscriptions.push(
        vscode.debug.registerDebugAdapterDescriptorFactory('antide-debug', {
            createDebugAdapterDescriptor(_session) {
                // This tells VS Code to run: antide dbg <filename>
                // We use DebugAdapterExecutable to launch the C binary directly
                return new vscode.DebugAdapterExecutable(debuggerPath, [
                    'dbg', 
                    _session.configuration.program
                ]);
            }
        })
    );

	context.subscriptions.push(
        vscode.debug.registerDebugConfigurationProvider('antide-debug', {
            provideDebugConfigurations(folder, token) {
                return [
                    {
                        type: 'antide-debug',
                        name: 'Launch Antide',
                        request: 'launch',
                        program: '${file}'
                    }
                ];
            },
            resolveDebugConfiguration(folder, config, token) {
                // If the user hasn't created a launch.json, config will be empty
                if (!config.type && !config.request && !config.name) {
                    const editor = vscode.window.activeTextEditor;
                    if (editor && editor.document.languageId === 'aasm') {
                        config.type = 'antide-debug';
                        config.name = 'Launch';
                        config.request = 'launch';
                        config.program = '${file}';
                    }
                }

                if (!config.program) {
                    return vscode.window.showInformationMessage(`Cannot find a program to debug ${JSON.stringify(config)}`).then(_ => {
                        return undefined; // abort launch
                    });
                }

                return config;
            }
        })
    );
	var grid_panel:any = null;

	context.subscriptions.push(
		vscode.debug.onDidReceiveDebugSessionCustomEvent(event => {
			if (event.event === 'gridContent') {
				// Forward the data to your Webview
				if (grid_panel == null)
				{
					grid_panel = vscode.window.createWebviewPanel(
						'antVisualizer',
						'Ant Colony Grid',
						vscode.ViewColumn.Beside, // Open it to the side of the code
						{ 
							enableScripts: true,
							localResourceRoots: [vscode.Uri.joinPath(context.extensionUri, 'media')]
						}
					);
				}
				grid_panel.webview.html = getGridWebviewContent(context); 
				grid_panel.webview.postMessage({
					command: 'gridContent',
					data: event.body
				});
			}
		})
	);

}

export function deactivate(): Thenable<void> | undefined {
	if (!client) {
		return undefined;
	}
	return client.stop();
}
