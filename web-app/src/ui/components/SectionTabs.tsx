export interface SectionTab {
  id: string;
  label: string;
}

interface SectionTabsProps {
  tabs: SectionTab[];
  active: string;
  onChange: (id: string) => void;
}

export function SectionTabs({ tabs, active, onChange }: SectionTabsProps) {
  return (
    <div className="section-tabs" role="tablist" aria-label="Секции настроек">
      {tabs.map((tab) => (
        <button
          key={tab.id}
          type="button"
          role="tab"
          aria-selected={active === tab.id}
          className={active === tab.id ? 'section-tab active' : 'section-tab'}
          onClick={() => onChange(tab.id)}
        >
          {tab.label}
        </button>
      ))}
    </div>
  );
}
