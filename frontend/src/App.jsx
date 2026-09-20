import React, { useState, useEffect } from 'react';
import { ResponsiveContainer, LineChart, Line, XAxis, YAxis, Tooltip } from 'recharts';
import { MapContainer, TileLayer, CircleMarker, Popup } from 'react-leaflet';
import 'leaflet/dist/leaflet.css';

export default function App() {
  const [user, setUser] = useState(null);
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [activeTab, setActiveTab] = useState('dashboard');
  const [menuOpen, setMenuOpen] = useState(false);

  // Real-time telemetry state
  const [telemetry, setTelemetry] = useState({
    water_level: 0.00,
    rainfall: 0.0,
    temperature: 31.6,
    pressure: 1004.8,
    humidity: 78,
    aqi: 42,
    risk_state: 'NORMAL',
    risk_probability: 12,
    ai_confidence: 94,
    active_sensors: '6/6',
    node_battery: 84
  });

  const [historyData, setHistoryData] = useState([
    { time: '12:00:00', risk: 10 },
    { time: '12:00:05', risk: 12 },
    { time: '12:00:10', risk: 11 },
    { time: '12:00:15', risk: 14 }
  ]);

  const mapCenter = [22.5726, 88.3639];

  const handleLogin = (e) => {
    e?.preventDefault();
    const role = email.includes('gov.in') || email.includes('authority') ? 'authority' : 'public';
    setUser({ email, role });
    setActiveTab('dashboard');
  };

  // Continuous polling hook: updates numbers & graph every 2 seconds
  useEffect(() => {
    if (!user) return;
    const interval = setInterval(async () => {
      try {
        const res = await fetch('http://localhost:8000/api/current');
        if (res.ok) {
          const data = await res.json();
          setTelemetry(prev => ({
            ...prev,
            water_level: data.water_level !== undefined ? data.water_level : prev.water_level,
            rainfall: data.rainfall !== undefined ? data.rainfall : (data.rainfall_rate ?? prev.rainfall),
            temperature: data.temperature !== undefined ? data.temperature : prev.temperature,
            pressure: data.pressure !== undefined ? data.pressure : prev.pressure,
            humidity: data.humidity !== undefined ? data.humidity : prev.humidity,
            risk_state: data.risk_state || prev.risk_state,
            risk_probability: Math.round(data.fused_score ?? prev.risk_probability),
            node_battery: data.node_battery ?? prev.node_battery
          }));

          setHistoryData(prev => {
            const now = new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
            const newPoint = { time: now, risk: Math.round(data.fused_score ?? 12) };
            return [...prev.slice(-6), newPoint];
          });
        }
      } catch (err) {
        console.log('Telemetry stream polling...');
      }
    }, 2000);
    return () => clearInterval(interval);
  }, [user]);

  // --- SIGN IN VIEW ---
  if (!user) {
    return (
      <div style={{
        minHeight: '100vh',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
        backgroundColor: '#0c151d',
        fontFamily: 'system-ui, -apple-system, sans-serif'
      }}>
        <div style={{
          width: '380px',
          padding: '36px',
          backgroundColor: '#16222f',
          borderRadius: '16px',
          boxShadow: '0 20px 40px rgba(0,0,0,0.5)',
          color: '#fff'
        }}>
          <div style={{ textAlign: 'center', marginBottom: '24px' }}>
            <div style={{
              width: '46px',
              height: '46px',
              backgroundColor: '#1b3b2b',
              color: '#34d399',
              fontSize: '22px',
              fontWeight: 'bold',
              display: 'inline-flex',
              alignItems: 'center',
              justifyContent: 'center',
              borderRadius: '10px',
              marginBottom: '10px'
            }}>P</div>
            <h2 style={{ margin: 0, fontSize: '20px', letterSpacing: '1px' }}>PRAKRITI</h2>
            <p style={{ margin: '4px 0 0 0', fontSize: '12px', color: '#94a3b8' }}>Real-time environmental intelligence system</p>
          </div>

          <form onSubmit={handleLogin} style={{ display: 'flex', flexDirection: 'column', gap: '14px' }}>
            <div>
              <label style={{ fontSize: '12px', color: '#cbd5e1' }}>Email</label>
              <input
                type="email"
                required
                placeholder="officer@ndma.gov.in / citizen@gmail.com"
                value={email}
                onChange={(e) => setEmail(e.target.value)}
                style={{
                  width: '100%',
                  boxSizing: 'border-box',
                  padding: '10px',
                  marginTop: '5px',
                  borderRadius: '6px',
                  background: '#0c151d',
                  border: '1px solid #334155',
                  color: '#fff'
                }}
              />
            </div>
            <div>
              <label style={{ fontSize: '12px', color: '#cbd5e1' }}>Password</label>
              <input
                type="password"
                required
                placeholder="••••••••"
                value={password}
                onChange={(e) => setPassword(e.target.value)}
                style={{
                  width: '100%',
                  boxSizing: 'border-box',
                  padding: '10px',
                  marginTop: '5px',
                  borderRadius: '6px',
                  background: '#0c151d',
                  border: '1px solid #334155',
                  color: '#fff'
                }}
              />
            </div>
            <button
              type="submit"
              style={{
                padding: '11px',
                marginTop: '6px',
                borderRadius: '6px',
                border: 'none',
                backgroundColor: '#0284c7',
                color: '#fff',
                fontWeight: '600',
                cursor: 'pointer'
              }}
            >
              Sign In
            </button>
          </form>

          <div style={{ marginTop: '20px', display: 'flex', gap: '8px' }}>
            <button
              onClick={() => { setEmail('officer@ndma.gov.in'); setPassword('123456'); }}
              style={{ flex: 1, padding: '7px', fontSize: '11px', background: '#0284c722', color: '#38bdf8', border: '1px solid #0284c755', borderRadius: '5px', cursor: 'pointer' }}
            >
              Demo Authority
            </button>
            <button
              onClick={() => { setEmail('citizen@gmail.com'); setPassword('123456'); }}
              style={{ flex: 1, padding: '7px', fontSize: '11px', background: '#10b98122', color: '#34d399', border: '1px solid #10b98155', borderRadius: '5px', cursor: 'pointer' }}
            >
              Demo Public
            </button>
          </div>
        </div>
      </div>
    );
  }

  const isAuthority = user.role === 'authority';

  const navItems = [
    { id: 'dashboard', label: 'Dashboard', icon: '📊' },
    { id: 'risk_map', label: 'Risk Map', icon: '🗺️' },
    ...(isAuthority ? [{ id: 'sensors', label: 'Sensors', icon: '📡' }] : []),
    { id: 'alerts', label: 'Alerts', icon: '⚠️' },
    ...(isAuthority ? [{ id: 'node_health', label: 'Node Health', icon: '📶' }] : [])
  ];

  return (
    <div style={{ minHeight: '100vh', backgroundColor: '#edf2f7', fontFamily: 'system-ui, -apple-system, sans-serif' }}>
      {/* --- TOP HORIZONTAL NAVIGATION BAR WITH DROPDOWN --- */}
      <header style={{
        backgroundColor: '#0e1724',
        color: '#fff',
        padding: '12px 32px',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'space-between',
        boxShadow: '0 2px 8px rgba(0,0,0,0.15)',
        position: 'sticky',
        top: 0,
        zIndex: 1000
      }}>
        {/* Brand */}
        <div style={{ display: 'flex', alignItems: 'center', gap: '14px' }}>
          <div style={{
            width: '34px',
            height: '34px',
            backgroundColor: '#143c2c',
            color: '#22c55e',
            borderRadius: '8px',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            fontWeight: 'bold',
            fontSize: '16px'
          }}>
            P
          </div>
          <div>
            <div style={{ fontSize: '14px', fontWeight: '800', letterSpacing: '1px' }}>PRAKRITI</div>
            <div style={{ fontSize: '10px', color: '#64748b' }}>Environmental Intelligence</div>
          </div>
        </div>

        {/* Modules Menu Dropdown */}
        <div style={{ position: 'relative' }}>
          <button
            onClick={() => setMenuOpen(!menuOpen)}
            style={{
              display: 'flex',
              alignItems: 'center',
              gap: '10px',
              padding: '9px 18px',
              backgroundColor: menuOpen ? '#1b2a3a' : '#16222f',
              color: '#38bdf8',
              border: '1px solid #334155',
              borderRadius: '8px',
              fontSize: '13px',
              fontWeight: '600',
              cursor: 'pointer'
            }}
          >
            <span>☰ Modules Menu: <strong>{navItems.find(i => i.id === activeTab)?.label}</strong></span>
            <span style={{ fontSize: '10px' }}>{menuOpen ? '▲' : '▼'}</span>
          </button>

          {menuOpen && (
            <div style={{
              position: 'absolute',
              top: '46px',
              left: 0,
              width: '210px',
              backgroundColor: '#16222f',
              border: '1px solid #334155',
              borderRadius: '8px',
              boxShadow: '0 10px 25px rgba(0,0,0,0.5)',
              padding: '6px',
              display: 'flex',
              flexDirection: 'column',
              gap: '4px',
              zIndex: 1100
            }}>
              {navItems.map(item => (
                <button
                  key={item.id}
                  onClick={() => {
                    setActiveTab(item.id);
                    setMenuOpen(false);
                  }}
                  style={{
                    display: 'flex',
                    alignItems: 'center',
                    gap: '10px',
                    padding: '9px 14px',
                    borderRadius: '6px',
                    border: 'none',
                    textAlign: 'left',
                    fontSize: '13px',
                    cursor: 'pointer',
                    backgroundColor: activeTab === item.id ? '#0284c7' : 'transparent',
                    color: activeTab === item.id ? '#fff' : '#cbd5e1',
                    fontWeight: activeTab === item.id ? '600' : 'normal'
                  }}
                >
                  <span>{item.icon}</span> {item.label}
                </button>
              ))}
            </div>
          )}
        </div>

        {/* System Online & Logout */}
        <div style={{ display: 'flex', alignItems: 'center', gap: '16px' }}>
          <div style={{ fontSize: '11px', color: '#10b981', display: 'flex', alignItems: 'center', gap: '6px' }}>
            <span style={{ width: '8px', height: '8px', borderRadius: '50%', backgroundColor: '#10b981' }}></span>
            System Online
          </div>
          <button
            onClick={() => setUser(null)}
            style={{
              padding: '6px 14px',
              fontSize: '12px',
              background: '#1e293b',
              border: '1px solid #334155',
              color: '#cbd5e1',
              borderRadius: '6px',
              cursor: 'pointer'
            }}
          >
            Logout ({isAuthority ? 'Authority' : 'Public'})
          </button>
        </div>
      </header>

      {/* --- DASHBOARD CONTENT --- */}
      <main style={{ padding: '24px 40px' }}>
        {/* Header */}
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '22px' }}>
          <div>
            <div style={{ fontSize: '11px', fontWeight: 'bold', letterSpacing: '1px', color: '#64748b', textTransform: 'uppercase' }}>
              {isAuthority ? 'AUTHORITY DASHBOARD' : 'PUBLIC ADVISORY DASHBOARD'}
            </div>
            <h1 style={{ margin: '2px 0 0 0', fontSize: '24px', color: '#0f172a' }}>Environmental Risk Monitor</h1>
            <p style={{ margin: 0, fontSize: '13px', color: '#64748b' }}>Real-time environmental intelligence and early warning system</p>
          </div>
          <div style={{ display: 'flex', alignItems: 'center', gap: '12px', fontSize: '12px', color: '#64748b' }}>
            <span>📍 Monitoring Zone 01</span>
            <span style={{ color: '#ef4444', fontWeight: 'bold' }}>● LIVE</span>
          </div>
        </div>

        {/* TAB 1: DASHBOARD */}
        {activeTab === 'dashboard' && (
          <div>
            {/* Top 3 Metric Cards */}
            <div style={{ display: 'grid', gridTemplateColumns: '1.4fr 1fr 1fr', gap: '18px', marginBottom: '20px' }}>
              <div style={{ backgroundColor: '#fff', borderRadius: '12px', padding: '18px 24px', border: '1px solid #e2e8f0', boxShadow: '0 1px 3px rgba(0,0,0,0.04)' }}>
                <span style={{ fontSize: '11px', fontWeight: 'bold', color: '#64748b', letterSpacing: '0.8px', textTransform: 'uppercase' }}>
                  OVERALL ENVIRONMENT RISK
                </span>
                <div style={{ display: 'flex', alignItems: 'baseline', justifyContent: 'space-between', marginTop: '6px' }}>
                  <span style={{ fontSize: '26px', fontWeight: 'bold', color: telemetry.risk_state === 'CRITICAL' ? '#ef4444' : telemetry.risk_state === 'WATCH' ? '#f59e0b' : '#10b981' }}>
                    {telemetry.risk_state}
                  </span>
                  <span style={{ fontSize: '26px', fontWeight: 'bold', color: '#0f172a' }}>{telemetry.risk_probability}%</span>
                </div>
                <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '11px', color: '#94a3b8', marginTop: '2px' }}>
                  <span>{telemetry.risk_state === 'NORMAL' ? 'Conditions baseline normal' : 'Conditions showing active risk indicators'}</span>
                  <span>Risk Probability</span>
                </div>
              </div>

              <div style={{ backgroundColor: '#fff', borderRadius: '12px', padding: '18px 24px', border: '1px solid #e2e8f0', boxShadow: '0 1px 3px rgba(0,0,0,0.04)' }}>
                <span style={{ fontSize: '11px', fontWeight: 'bold', color: '#64748b', letterSpacing: '0.8px', textTransform: 'uppercase' }}>
                  AI CONFIDENCE
                </span>
                <div style={{ fontSize: '26px', fontWeight: 'bold', color: '#0f172a', marginTop: '6px' }}>
                  {telemetry.ai_confidence}%
                </div>
                <div style={{ fontSize: '11px', color: '#10b981', marginTop: '2px' }}>
                  ✓ High confidence
                </div>
              </div>

              <div style={{ backgroundColor: '#fff', borderRadius: '12px', padding: '18px 24px', border: '1px solid #e2e8f0', boxShadow: '0 1px 3px rgba(0,0,0,0.04)' }}>
                <span style={{ fontSize: '11px', fontWeight: 'bold', color: '#64748b', letterSpacing: '0.8px', textTransform: 'uppercase' }}>
                  ACTIVE SENSORS
                </span>
                <div style={{ fontSize: '26px', fontWeight: 'bold', color: '#0f172a', marginTop: '6px' }}>
                  {telemetry.active_sensors}
                </div>
                <div style={{ fontSize: '11px', color: '#10b981', marginTop: '2px' }}>
                  ✓ All systems operational
                </div>
              </div>
            </div>

            {/* Environmental Sensors Header */}
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '12px' }}>
              <div>
                <span style={{ fontSize: '11px', color: '#0284c7', fontWeight: 'bold', letterSpacing: '0.8px', textTransform: 'uppercase' }}>LIVE DATA</span>
                <h3 style={{ margin: 0, fontSize: '16px', color: '#0f172a' }}>Environmental Sensors</h3>
              </div>
              <span style={{ fontSize: '11px', color: '#94a3b8' }}>Updated live via InfluxDB / LoRa</span>
            </div>

            {/* 4 Sensor Cards */}
            <div style={{ display: 'grid', gridTemplateColumns: 'repeat(4, 1fr)', gap: '16px', marginBottom: '22px' }}>
              <div style={{ backgroundColor: '#fff', padding: '18px', borderRadius: '10px', border: '1px solid #e2e8f0' }}>
                <div style={{ width: '28px', height: '28px', backgroundColor: '#e0f2fe', color: '#0284c7', borderRadius: '6px', display: 'flex', alignItems: 'center', justifyContent: 'center', fontSize: '14px', marginBottom: '10px' }}>💧</div>
                <span style={{ fontSize: '11px', color: '#64748b' }}>Water Level</span>
                <div style={{ fontSize: '22px', fontWeight: 'bold', color: '#0f172a', margin: '4px 0' }}>{telemetry.water_level} <span style={{ fontSize: '13px', fontWeight: 'normal' }}>m</span></div>
                <div style={{ fontSize: '11px', color: telemetry.water_level > 0.5 ? '#f59e0b' : '#10b981' }}>
                  {telemetry.water_level > 0.5 ? '↗ Rising' : '✓ Normal Baseline'}
                </div>
              </div>

              <div style={{ backgroundColor: '#fff', padding: '18px', borderRadius: '10px', border: '1px solid #e2e8f0' }}>
                <div style={{ width: '28px', height: '28px', backgroundColor: '#e0f2fe', color: '#0284c7', borderRadius: '6px', display: 'flex', alignItems: 'center', justifyContent: 'center', fontSize: '14px', marginBottom: '10px' }}>🌧️</div>
                <span style={{ fontSize: '11px', color: '#64748b' }}>Rainfall</span>
                <div style={{ fontSize: '22px', fontWeight: 'bold', color: '#0f172a', margin: '4px 0' }}>{telemetry.rainfall} <span style={{ fontSize: '13px', fontWeight: 'normal' }}>mm/hr</span></div>
                <div style={{ fontSize: '11px', color: telemetry.rainfall > 20 ? '#ef4444' : '#10b981' }}>
                  {telemetry.rainfall > 20 ? 'High intensity' : 'Low / Normal'}
                </div>
              </div>

              <div style={{ backgroundColor: '#fff', padding: '18px', borderRadius: '10px', border: '1px solid #e2e8f0' }}>
                <div style={{ width: '28px', height: '28px', backgroundColor: '#fee2e2', color: '#ef4444', borderRadius: '6px', display: 'flex', alignItems: 'center', justifyContent: 'center', fontSize: '14px', marginBottom: '10px' }}>🌡️</div>
                <span style={{ fontSize: '11px', color: '#64748b' }}>Temperature</span>
                <div style={{ fontSize: '22px', fontWeight: 'bold', color: '#0f172a', margin: '4px 0' }}>{telemetry.temperature} <span style={{ fontSize: '13px', fontWeight: 'normal' }}>°C</span></div>
                <div style={{ fontSize: '11px', color: '#10b981' }}>Normal</div>
              </div>

              <div style={{ backgroundColor: '#fff', padding: '18px', borderRadius: '10px', border: '1px solid #e2e8f0' }}>
                <div style={{ width: '28px', height: '28px', backgroundColor: '#f1f5f9', color: '#64748b', borderRadius: '6px', display: 'flex', alignItems: 'center', justifyContent: 'center', fontSize: '14px', marginBottom: '10px' }}>🧭</div>
                <span style={{ fontSize: '11px', color: '#64748b' }}>Pressure</span>
                <div style={{ fontSize: '22px', fontWeight: 'bold', color: '#0f172a', margin: '4px 0' }}>{telemetry.pressure} <span style={{ fontSize: '13px', fontWeight: 'normal' }}>hPa</span></div>
                <div style={{ fontSize: '11px', color: '#64748b' }}>Stable</div>
              </div>
            </div>

            {/* Bottom Row: Dynamic Graph & Role Module */}
            <div style={{ display: 'grid', gridTemplateColumns: '1.4fr 1fr', gap: '20px' }}>
              <div style={{ backgroundColor: '#fff', padding: '18px', borderRadius: '12px', border: '1px solid #e2e8f0' }}>
                <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '12px' }}>
                  <div>
                    <span style={{ fontSize: '11px', color: '#64748b', fontWeight: 'bold' }}>RISK ANALYSIS</span>
                    <h4 style={{ margin: '2px 0 0 0', fontSize: '15px' }}>Live Risk Trend</h4>
                  </div>
                  <span style={{ fontSize: '12px', color: '#10b981', fontWeight: '600' }}>Streaming Live</span>
                </div>
                <div style={{ height: '170px' }}>
                  <ResponsiveContainer width="100%" height="100%">
                    <LineChart data={historyData}>
                      <XAxis dataKey="time" stroke="#94a3b8" fontSize={11} />
                      <YAxis domain={[0, 100]} stroke="#94a3b8" fontSize={11} />
                      <Tooltip />
                      <Line type="monotone" dataKey="risk" stroke="#10b981" strokeWidth={2} dot={{ r: 3 }} />
                    </LineChart>
                  </ResponsiveContainer>
                </div>
              </div>

              {isAuthority ? (
                <div style={{ backgroundColor: '#fff', padding: '18px', borderRadius: '12px', border: '1px solid #e2e8f0', display: 'flex', flexDirection: 'column', justifyContent: 'space-between' }}>
                  <div>
                    <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '12px' }}>
                      <span style={{ fontSize: '11px', color: '#64748b', fontWeight: 'bold' }}>NETWORK</span>
                      <span style={{ fontSize: '11px', color: '#10b981', fontWeight: 'bold', background: '#dcfce7', padding: '2px 8px', borderRadius: '4px' }}>Healthy</span>
                    </div>
                    <h4 style={{ margin: '0 0 14px 0', fontSize: '15px' }}>Node Health</h4>
                    <div style={{ display: 'flex', flexDirection: 'column', gap: '10px', fontSize: '13px' }}>
                      <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                        <span style={{ color: '#64748b' }}>📶 LoRa Link</span>
                        <span style={{ fontWeight: '600', color: '#10b981' }}>Connected</span>
                      </div>
                      <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                        <span style={{ color: '#64748b' }}>🔋 Node 1 Battery</span>
                        <span style={{ fontWeight: '600' }}>{telemetry.node_battery}%</span>
                      </div>
                      <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                        <span style={{ color: '#64748b' }}>⚙️ Sensor Health</span>
                        <span style={{ fontWeight: '600' }}>100%</span>
                      </div>
                    </div>
                  </div>
                </div>
              ) : (
                <div style={{ backgroundColor: '#fff', padding: '18px', borderRadius: '12px', border: '1px solid #e2e8f0' }}>
                  <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '8px' }}>
                    <span style={{ fontSize: '11px', color: '#64748b', fontWeight: 'bold' }}>EMERGENCY RESPONSE</span>
                    <span style={{ fontSize: '11px', color: '#ef4444', fontWeight: 'bold', background: '#fee2e2', padding: '2px 8px', borderRadius: '4px' }}>24x7 India</span>
                  </div>
                  <h4 style={{ margin: '0 0 12px 0', fontSize: '15px', color: '#0f766e' }}>National Helplines</h4>
                  <div style={{ display: 'flex', flexDirection: 'column', gap: '8px', fontSize: '12px' }}>
                    <div style={{ display: 'flex', justifyContent: 'space-between', padding: '6px', background: '#f8fafc', borderRadius: '4px' }}>
                      <span>National Emergency (All-in-One)</span>
                      <strong style={{ color: '#b91c1c' }}>112</strong>
                    </div>
                    <div style={{ display: 'flex', justifyContent: 'space-between', padding: '6px', background: '#f8fafc', borderRadius: '4px' }}>
                      <span>NDMA / NDRF Disaster Line</span>
                      <strong style={{ color: '#0284c7' }}>1078</strong>
                    </div>
                    <div style={{ display: 'flex', justifyContent: 'space-between', padding: '6px', background: '#f8fafc', borderRadius: '4px' }}>
                      <span>District Disaster Control Room</span>
                      <strong style={{ color: '#f97316' }}>1077</strong>
                    </div>
                    <div style={{ display: 'flex', justifyContent: 'space-between', padding: '6px', background: '#f8fafc', borderRadius: '4px' }}>
                      <span>State Disaster Control (SDMA)</span>
                      <strong style={{ color: '#10b981' }}>1070</strong>
                    </div>
                  </div>
                </div>
              )}
            </div>
          </div>
        )}

        {/* TAB 2: RISK MAP */}
        {activeTab === 'risk_map' && (
          <div style={{ backgroundColor: '#fff', padding: '22px', borderRadius: '12px', border: '1px solid #e2e8f0' }}>
            <div style={{ marginBottom: '16px' }}>
              <span style={{ fontSize: '11px', color: '#64748b', fontWeight: 'bold', letterSpacing: '0.8px', textTransform: 'uppercase' }}>GEOSPATIAL INTELLIGENCE</span>
              <h3 style={{ margin: '2px 0 0 0', fontSize: '18px' }}>Regional Risk Map</h3>
              <p style={{ margin: 0, fontSize: '12px', color: '#64748b' }}>Monitor environmental risk zones and Node 1 location</p>
            </div>
            <div style={{ height: '480px', borderRadius: '8px', overflow: 'hidden' }}>
              <MapContainer center={mapCenter} zoom={11} style={{ height: '100%', width: '100%' }}>
                <TileLayer url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png" />
                <CircleMarker center={mapCenter} radius={16} pathOptions={{ color: '#10b981', fillColor: '#10b981', fillOpacity: 0.8 }}>
                  <Popup>
                    <strong>Node 1 (Hooghly / Kolkata Regional Site)</strong><br />
                    State: {telemetry.risk_state}<br />
                    Water Level: {telemetry.water_level} m<br />
                    Rainfall: {telemetry.rainfall} mm/hr
                  </Popup>
                </CircleMarker>
              </MapContainer>
            </div>
          </div>
        )}

        {/* TAB 3: SENSORS (AUTHORITY ONLY) */}
        {activeTab === 'sensors' && isAuthority && (
          <div>
            <div style={{ marginBottom: '16px' }}>
              <span style={{ fontSize: '11px', color: '#64748b', fontWeight: 'bold', letterSpacing: '0.8px', textTransform: 'uppercase' }}>NODE 1</span>
              <h3 style={{ margin: '2px 0 0 0', fontSize: '18px' }}>Sensor Monitoring Matrix</h3>
            </div>
            <div style={{ display: 'grid', gridTemplateColumns: 'repeat(4, 1fr)', gap: '16px', marginBottom: '16px' }}>
              <div style={{ backgroundColor: '#fff', padding: '18px', borderRadius: '10px', border: '1px solid #e2e8f0' }}>
                <div style={{ width: '28px', height: '28px', backgroundColor: '#e0f2fe', color: '#0284c7', borderRadius: '6px', display: 'flex', alignItems: 'center', justifyContent: 'center', marginBottom: '8px' }}>💧</div>
                <span style={{ fontSize: '11px', color: '#64748b' }}>Water Level</span>
                <div style={{ fontSize: '22px', fontWeight: 'bold', color: '#0f172a', margin: '4px 0' }}>{telemetry.water_level} m</div>
                <div style={{ fontSize: '11px', color: telemetry.water_level > 0.5 ? '#f59e0b' : '#10b981' }}>{telemetry.water_level > 0.5 ? '↗ Rising' : '✓ Normal'}</div>
              </div>
              <div style={{ backgroundColor: '#fff', padding: '18px', borderRadius: '10px', border: '1px solid #e2e8f0' }}>
                <div style={{ width: '28px', height: '28px', backgroundColor: '#e0f2fe', color: '#0284c7', borderRadius: '6px', display: 'flex', alignItems: 'center', justifyContent: 'center', marginBottom: '8px' }}>🌧️</div>
                <span style={{ fontSize: '11px', color: '#64748b' }}>Rainfall</span>
                <div style={{ fontSize: '22px', fontWeight: 'bold', color: '#0f172a', margin: '4px 0' }}>{telemetry.rainfall} mm/hr</div>
                <div style={{ fontSize: '11px', color: '#10b981' }}>Active Feed</div>
              </div>
              <div style={{ backgroundColor: '#fff', padding: '18px', borderRadius: '10px', border: '1px solid #e2e8f0' }}>
                <div style={{ width: '28px', height: '28px', backgroundColor: '#fee2e2', color: '#ef4444', borderRadius: '6px', display: 'flex', alignItems: 'center', justifyContent: 'center', marginBottom: '8px' }}>🌡️</div>
                <span style={{ fontSize: '11px', color: '#64748b' }}>Temperature</span>
                <div style={{ fontSize: '22px', fontWeight: 'bold', color: '#0f172a', margin: '4px 0' }}>{telemetry.temperature} °C</div>
                <div style={{ fontSize: '11px', color: '#10b981' }}>Calibrated</div>
              </div>
              <div style={{ backgroundColor: '#fff', padding: '18px', borderRadius: '10px', border: '1px solid #e2e8f0' }}>
                <div style={{ width: '28px', height: '28px', backgroundColor: '#f1f5f9', color: '#64748b', borderRadius: '6px', display: 'flex', alignItems: 'center', justifyContent: 'center', marginBottom: '8px' }}>🧭</div>
                <span style={{ fontSize: '11px', color: '#64748b' }}>Pressure</span>
                <div style={{ fontSize: '22px', fontWeight: 'bold', color: '#0f172a', margin: '4px 0' }}>{telemetry.pressure} hPa</div>
                <div style={{ fontSize: '11px', color: '#64748b' }}>Stable</div>
              </div>
            </div>

            <div style={{ display: 'grid', gridTemplateColumns: 'repeat(4, 1fr)', gap: '16px' }}>
              <div style={{ backgroundColor: '#fff', padding: '18px', borderRadius: '10px', border: '1px solid #e2e8f0' }}>
                <div style={{ width: '28px', height: '28px', backgroundColor: '#ecfdf5', color: '#10b981', borderRadius: '6px', display: 'flex', alignItems: 'center', justifyContent: 'center', marginBottom: '8px' }}>💨</div>
                <span style={{ fontSize: '11px', color: '#64748b' }}>Humidity</span>
                <div style={{ fontSize: '22px', fontWeight: 'bold', color: '#0f172a', margin: '4px 0' }}>{telemetry.humidity} %</div>
                <div style={{ fontSize: '11px', color: '#10b981' }}>Optimal</div>
              </div>
              <div style={{ backgroundColor: '#fff', padding: '18px', borderRadius: '10px', border: '1px solid #e2e8f0' }}>
                <div style={{ width: '28px', height: '28px', backgroundColor: '#f8fafc', color: '#64748b', borderRadius: '6px', display: 'flex', alignItems: 'center', justifyContent: 'center', marginBottom: '8px' }}>🌫️</div>
                <span style={{ fontSize: '11px', color: '#64748b' }}>Air Quality</span>
                <div style={{ fontSize: '22px', fontWeight: 'bold', color: '#0f172a', margin: '4px 0' }}>{telemetry.aqi} AQI</div>
                <div style={{ fontSize: '11px', color: '#10b981' }}>Moderate</div>
              </div>
            </div>
          </div>
        )}

        {/* TAB 4: ALERTS */}
        {activeTab === 'alerts' && (
          <div style={{ backgroundColor: '#fff', padding: '22px', borderRadius: '12px', border: '1px solid #e2e8f0' }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '16px' }}>
              <div>
                <span style={{ fontSize: '11px', color: '#64748b', fontWeight: 'bold', letterSpacing: '0.8px', textTransform: 'uppercase' }}>EARLY WARNING SYSTEM</span>
                <h3 style={{ margin: '2px 0 0 0', fontSize: '18px' }}>Active Alerts</h3>
              </div>
              <span style={{ fontSize: '11px', backgroundColor: telemetry.risk_state === 'NORMAL' ? '#dcfce7' : '#fef3c7', color: telemetry.risk_state === 'NORMAL' ? '#166534' : '#b45309', padding: '4px 10px', borderRadius: '12px', fontWeight: 'bold' }}>
                {telemetry.risk_state === 'NORMAL' ? '0 Critical Hazards' : 'Active Warning'}
              </span>
            </div>

            <div style={{ display: 'flex', flexDirection: 'column', gap: '14px' }}>
              <div style={{ padding: '16px', borderRadius: '8px', backgroundColor: '#f8fafc', border: '1px solid #e2e8f0' }}>
                <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                  <span style={{ fontWeight: 'bold', color: '#0f172a', fontSize: '14px' }}>📡 Telemetry Stream Active</span>
                  <span style={{ fontSize: '11px', fontWeight: 'bold', color: '#10b981' }}>NORMAL</span>
                </div>
                <p style={{ margin: '6px 0 0 0', fontSize: '13px', color: '#64748b' }}>
                  Water level is currently {telemetry.water_level} m with precipitation at {telemetry.rainfall} mm/hr. System baseline normal.
                </p>
              </div>
            </div>
          </div>
        )}

        {/* TAB 5: NODE HEALTH (AUTHORITY ONLY) */}
        {activeTab === 'node_health' && isAuthority && (
          <div style={{ backgroundColor: '#fff', padding: '22px', borderRadius: '12px', border: '1px solid #e2e8f0' }}>
            <div style={{ marginBottom: '20px' }}>
              <span style={{ fontSize: '11px', color: '#64748b', fontWeight: 'bold', letterSpacing: '0.8px', textTransform: 'uppercase' }}>NETWORK STATUS</span>
              <h3 style={{ margin: '2px 0 0 0', fontSize: '18px' }}>Node Health</h3>
            </div>

            <div style={{ display: 'flex', flexDirection: 'column', gap: '14px', maxWidth: '600px', fontSize: '14px' }}>
              <div style={{ display: 'flex', justifyContent: 'space-between', paddingBottom: '10px', borderBottom: '1px solid #f1f5f9' }}>
                <span style={{ color: '#64748b' }}>📶 LoRa Link</span>
                <span style={{ color: '#10b981', fontWeight: 'bold' }}>Connected</span>
              </div>
              <div style={{ display: 'flex', justifyContent: 'space-between', paddingBottom: '10px', borderBottom: '1px solid #f1f5f9' }}>
                <span style={{ color: '#64748b' }}>🔋 Node 1 Battery</span>
                <span style={{ color: '#0f172a', fontWeight: 'bold' }}>{telemetry.node_battery}%</span>
              </div>
              <div style={{ display: 'flex', justifyContent: 'space-between', paddingBottom: '10px', borderBottom: '1px solid #f1f5f9' }}>
                <span style={{ color: '#64748b' }}>⚙️ Sensor Health</span>
                <span style={{ color: '#10b981', fontWeight: 'bold' }}>100%</span>
              </div>
              <div style={{ display: 'flex', justifyContent: 'space-between', paddingBottom: '10px', borderBottom: '1px solid #f1f5f9' }}>
                <span style={{ color: '#64748b' }}>📡 Signal Strength</span>
                <span style={{ color: '#0f172a', fontWeight: 'bold' }}>-78 dBm (Strong)</span>
              </div>
              <div style={{ display: 'flex', justifyContent: 'space-between', paddingBottom: '10px', borderBottom: '1px solid #f1f5f9' }}>
                <span style={{ color: '#64748b' }}>⏱️ Sampling Rate</span>
                <span style={{ color: '#0f172a', fontWeight: 'bold' }}>2.0s / packet</span>
              </div>
              <div style={{ display: 'flex', justifyContent: 'space-between', paddingBottom: '10px', borderBottom: '1px solid #f1f5f9' }}>
                <span style={{ color: '#64748b' }}>🧠 AI Risk Engine</span>
                <span style={{ color: '#10b981', fontWeight: 'bold' }}>Operational</span>
              </div>
            </div>
          </div>
        )}
      </main>
    </div>
  );
}