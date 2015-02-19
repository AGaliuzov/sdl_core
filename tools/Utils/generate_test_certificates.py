#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""Generate certificates for testing
  Usage:
    generate_test_certificate.py --dir=<name of directory>
"""

import os
from argparse import ArgumentParser
from subprocess import check_call

def run(command, *args):
	"""Application caller
	wrap console call 'command args'
	"""
	args_str = " " + " ".join(str(s) for s in list(args[0]))
	if len(args_str) > 0 :
		command += " " + args_str
	print "Running", command
	retcode = check_call(command, shell=True)
	if retcode < 0:
		raise RuntimeError("Child was terminated by signal")

def openssl(*args):
	"""OpenSSL caller
	wrap console call 'openssl args'
	"""
	run("openssl", args)

def gen_rsa_key(out_key_file, key_size):
	"""Private key generator
	wrap console call 'openssl genrsa -out $out_key_file $key_size'
	"""
	openssl("genrsa", "-out", out_key_file, key_size)

def gen_root_cert(out_cert_file, key_file, days, answer):
	"""Root certificate generator
	wrap console call 'openssl req -x509 -new -key $key_file -days $days -out $out_cert_file -subj $answer'
	"""
	openssl("req -x509 -new -key", key_file, "-days", days, "-out", out_cert_file, "-subj", answer)

def gen_cert(out_cert_file, out_sign_cert_file, key_file, ca_cert_file, ca_key_file, days, answer):
	"""Certificate generator
	wrap console call
	'openssl req -new -key $key_file -days $days -out $out_cert_file -subj $answer'
	'openssl x509 -req -in $out_cert_file -CA $ca_cert_file -CAkey ca_key_file -CAcreateserial -out $out_sign_cert_file -days 5000'
	"""
	openssl("req -new -key", key_file, "-days", days, "-out", out_cert_file, "-subj", answer)

	openssl("x509 -req -in", out_cert_file, "-CA", ca_cert_file, "-CAkey", ca_key_file, \
		"-CAcreateserial -out", out_sign_cert_file, "-days", days)

def answers(name, country, state, locality, organization, unit, email) :
	"""Answer string generator
	Generate answer for certificate creation with openssl
	Country argument need to be 2 symbol size
	"""
	if len(country) != 2 :
		raise ValueError("Country argument need to be 2 symbol size")
	answer ="'/C={0}/ST={1}/L={2}/O={3}".format(country, state, locality, organization)
	answer +="/OU={0}/CN={1}/emailAddress={2}'".format(unit, name, email)
	return answer


def main():
	arg_parser = ArgumentParser(description='Welcome to SDL test certificate generator.')
	arg_parser.add_argument('-d', '--dir', help="directory for certificate generating")
	args = arg_parser.parse_args()
	if args.dir and not os.path.exists(args.dir) :
		raise OSError("Input directory not exists")
	dir = args.dir if args.dir else ""

	root_answer = answers("root", "US", "California", "Silicon Valley", "CAcert.org", "CAcert", "sample@cacert.org")
	ford_answer = answers("FORD", "US", "Michigan", "Detroit", "FORD", "FORD_SDL" ,"sample@ford.com")
	ford2_answer = answers("FORD", "US", "Michigan", "Detroit", "FORD2", "FORD_SDL2" ,"sample@ford.com")
	client_answer  = answers("client", "RU", "Russia", "St. Petersburg", "Luxoft", "HeadUnit" ,"sample@luxoft.com")
	server_answer  = answers("server", "RU", "Russia", "St. Petersburg", "Luxoft", "Mobile" ,"sample@luxoft.com")
	days = 10000

	print " --== Root certificate generating ==-- "
	root_key_file = os.path.join(dir, "root.key")
	root_cert_file = os.path.join(dir, "root.crt")
	gen_rsa_key(root_key_file, 2048)
	gen_root_cert(root_cert_file, root_key_file, days, root_answer)

	print
	print " --== Ford1 certificate generating ==-- "
	ford_key_file = os.path.join(dir, "ford.key")
	ford_cert_file = os.path.join(dir, "ford.crt")
	gen_rsa_key(ford_key_file, 2048)
	gen_cert(ford_cert_file, ford_cert_file, ford_key_file, root_cert_file, root_key_file, days, ford_answer)

	print
	print " --== Ford2 certificate generating ==-- "
	ford2_key_file = os.path.join(dir, "ford2.key")
	ford2_cert_file = os.path.join(dir, "ford2.crt")
	gen_rsa_key(ford2_key_file, 2048)
	gen_cert(ford2_cert_file, ford2_cert_file, ford2_key_file, root_cert_file, root_key_file, days, ford2_answer)

	print
	print " --== Server certificate generating ==-- "
	server_key_file = os.path.join(dir, "server.key")
	server_cert_file = os.path.join(dir, "server.crt")
	server_sign_cert_file = os.path.join(dir, "server_sign.crt")
	gen_rsa_key(server_key_file, 2048)
	gen_cert(server_cert_file, server_sign_cert_file, server_key_file, ford_cert_file, ford_key_file, days, client_answer)

	print
	print " --== Client certificate generating ==-- "
	client_key_file = os.path.join(dir, "client.key")
	client_cert_file = os.path.join(dir, "client.crt")
	client_sign_cert_file = os.path.join(dir, "client_sign.crt")
	gen_rsa_key(client_key_file, 2048)
	gen_cert(client_cert_file, client_sign_cert_file, client_key_file, ford2_cert_file, ford2_key_file, days, server_answer)

	print
	print "All certificate have generated"

if __name__ == "__main__":
	main()
