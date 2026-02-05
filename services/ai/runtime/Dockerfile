FROM python:3.12-slim

ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1

WORKDIR /opt/zenith

# Keep Python deps minimal; signaling/port mapper rely only on stdlib
COPY backend/requirements.txt ./backend/signaling/requirements.txt

COPY backend ./backend

RUN pip install --no-cache-dir -r backend/signaling/requirements.txt

EXPOSE 54320 54321 8000

ENTRYPOINT ["python", "-m", "backend.run_backend"]
