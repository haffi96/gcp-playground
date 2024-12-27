FROM python:3.11

WORKDIR /code


COPY ./chat_app/requirements.txt .
COPY ./chat_app/setup.py .


RUN pip install --no-cache-dir --upgrade -r requirements.txt \
    && pip install --no-cache-dir -e .


COPY ./chat_app .


CMD ["python", "app/main.py"]