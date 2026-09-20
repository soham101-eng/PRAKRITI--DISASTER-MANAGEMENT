from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
from typing import Optional

app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Global in-memory storage for real-time sensor telemetry
latest_telemetry = {
    "water_level": 0.00,
    "rainfall": 0.0,
    "rainfall_rate": 0.0,
    "temperature": 31.6,
    "pressure": 1004.8,
    "humidity": 78,
    "soil_moisture": 25.0,
    "risk_state": "NORMAL",
    "fused_score": 12.0,
    "node_battery": 84
}

class TelemetryPayload(BaseModel):
    node_id: Optional[int] = 1
    seq: Optional[int] = 1
    water_level: float
    rainfall: Optional[float] = 0.0
    pressure: Optional[float] = 1013.0
    temperature: Optional[float] = 25.0
    humidity: Optional[float] = 50.0
    soil_moisture: Optional[float] = 0.0
    fused_score: Optional[float] = 0.0
    risk_state: Optional[str] = "NORMAL"
    rssi: Optional[float] = -75.0

class LoginRequest(BaseModel):
    email: str
    password: str

@app.post("/api/auth/login")
async def login(req: LoginRequest):
    role = "authority" if ("gov.in" in req.email or "authority" in req.email) else "public"
    return {"email": req.email, "role": role}

@app.post("/api/telemetry")
async def receive_telemetry(data: TelemetryPayload):
    global latest_telemetry
    latest_telemetry = {
        "water_level": data.water_level,
        "rainfall": data.rainfall,
        "rainfall_rate": data.rainfall,
        "temperature": data.temperature,
        "pressure": data.pressure,
        "humidity": data.humidity,
        "soil_moisture": data.soil_moisture,
        "risk_state": data.risk_state,
        "fused_score": data.fused_score,
        "node_battery": 84
    }
    return {"status": "ok"}

@app.get("/api/current")
async def get_current():
    return latest_telemetry