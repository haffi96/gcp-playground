FROM python:3.9

WORKDIR /code


COPY ./chat-app/requirements.txt /code/requirements.txt


RUN pip install --no-cache-dir --upgrade -r /code/requirements.txt


COPY ./chat-app /code


CMD ["python", "main.py"]