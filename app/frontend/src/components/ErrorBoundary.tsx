import { Component, type ReactNode } from "react";

type Props = { children: ReactNode };
type State = { hasError: boolean; error?: any };

export default class ErrorBoundary extends Component<Props, State> {
  state: State = { hasError: false };
  static getDerivedStateFromError(error: any) {
    return { hasError: true, error };
  }
  render() {
    if (this.state.hasError) {
      return (
        <div className="p-4 text-red-700 bg-red-50 border border-red-200 rounded">
          <div className="font-semibold mb-1">A UI error occurred.</div>
          <pre className="text-xs overflow-auto">{String(this.state.error)}</pre>
        </div>
      );
    }
    return this.props.children;
  }
}

