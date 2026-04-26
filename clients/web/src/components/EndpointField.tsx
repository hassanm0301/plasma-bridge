interface EndpointFieldProps {
  id: string;
  label: string;
  value: string;
  placeholder: string;
  onChange: (value: string) => void;
}

export function EndpointField({ id, label, value, placeholder, onChange }: EndpointFieldProps) {
  return (
    <label className="endpoint-field" htmlFor={id}>
      <span>{label}</span>
      <input
        id={id}
        spellCheck="false"
        value={value}
        placeholder={placeholder}
        onChange={(event) => onChange(event.target.value)}
      />
    </label>
  );
}
