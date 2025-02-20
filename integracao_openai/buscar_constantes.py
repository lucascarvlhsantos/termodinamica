from openai import OpenAI

client = OpenAI(api_key="s<OpenAI API Key>")

def ler_dataset_do_arquivo(arquivo):
    with open(arquivo, 'r') as f:
        dataset = f.read() 
    return dataset

dataset = ler_dataset_do_arquivo('./dataset.txt')
prompt = f"""
Com base em um sistema de controle de temperatura onde o objetivo é minimizar o erro entre a temperatura medida e o setpoint, 
suponha que você tenha acesso a um modelo interno ou a dados simulados que representam a dinâmica do sistema. Com base nos dados fornecidos, faça uma análise de quais valores
de Kp resultaram no melhor desempenho para minimizar o erro. Além disso, tente uma previsão de, uma vez decidido o Kp, qual o melhor valor de Kd e Ki. Pode usar o método de Ziegler-Nichols.

{dataset}
"""

# Round 1: Primeiro envio de dados;
response = client.chat.completions.create(
    model="gpt-4",  
    messages=[{"role": "user", "content": prompt}]
)
content = response.choices[0].message.content
print("Constantes PID Otimizadas:", content)

# Round 2: enviar para a avaliação do outro modelo;
messages = [{"role": "user", "content": prompt}]
messages.append({'role': 'assistant', 'content': content})
messages.append({'role': 'user', 'content': "Baseado nos dados do sistema, julgue a resposta fornecida para definir as constante. Você concorda com a análise?"})

response_round2 = client.chat.completions.create(
    model="o3-mini",
    messages=messages
)
print("Resposta de Refinamento:", response_round2.choices[0].message.content)