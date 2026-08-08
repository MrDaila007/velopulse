import type { ReactNode } from 'react';

interface InfoCalloutProps {
  tone?: 'info' | 'warn';
  children: ReactNode;
}

export function InfoCallout({ tone = 'info', children }: InfoCalloutProps) {
  return <div className={`info-callout ${tone}`}>{children}</div>;
}
